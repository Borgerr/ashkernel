#include "kernel.h"
#include "../common.h"
#include "riscv.h"  // TODO: change depending on target

extern uint8_t __bss[], __bss_end[];	// taken from kernel.ld

struct proc procs[PROCS_MAX];

struct proc *current_proc;
struct proc *idle_proc;

// testing procs and prototypes for testing context switching
struct proc *proc_a;
struct proc *proc_b;
void proc_a_entry(void);
void proc_b_entry(void);

// checking loaded shell
extern char _binary_shell_bin_start[], _binary_shell_bin_size[];

void virtio_blk_init(void);     // XXX: can be made more sophisticated
void read_write_disk(void *buf, unsigned sector, bool is_write);
void fs_init(void);

void kernel_main(void)
{
	memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);  // set bss to 0 as a sanity check
    WRITE_CSR(stvec, (uint32_t) kernel_entry);

    virtio_blk_init();  // XXX: probably want to refactor
    fs_init();

    char buf[SECTOR_SIZE];
    read_write_disk(buf, 0, false /* read */);
    printf("first sector: %s\n", buf);
    strcpy(buf, "sup shawty\n");
    read_write_disk(buf, 0, true /* write */);

    printf("initializing idle process\n");
    idle_proc = init_proc(NULL, 0);
    idle_proc->pid = 0;
    current_proc = idle_proc;   // boot process context saved, can return to later

    printf("initializing loaded shell at addr: %d\n", (size_t) _binary_shell_bin_size);
    init_proc(_binary_shell_bin_start, (size_t) _binary_shell_bin_size);
    yield();

    /*
    proc_a = init_proc((uint32_t) proc_a_entry);
    proc_b = init_proc((uint32_t) proc_b_entry);

    uint8_t *shell_bin = (uint8_t *) _binary_shell_bin_start;
    */

    printf("nothing is running. it sure is boring around here.\n");

	for (;;)
        __asm__ __volatile__("wfi");    // FIXME: depends on riscv
}

/*
 * --------------------------------------------------------------------------------
 * MEMORY MANAGEMENT
 * --------------------------------------------------------------------------------
 */
// everything here should still apply regardless of arch
extern char __free_ram[], __free_ram_end[];

paddr_t alloc_pages(uint32_t n)
{
    /*
     * Allocates `n` pages
     * Simple bump allocator for now.
     * TODO: make more sophisticated-- we want high memory utilization
     * and ways to free pages.
     * RISC-V also likely has some MMU utility we can wrangle here for S-Mode vs M-Mode.
     * Also always panics on invalid requests-- may want to propagate an "invalid request" sort of signal instead for
     * a robust system.
     *
     * XXX: should this go in riscv.c?
     */
    if (n * PAGE_SIZE == 0 && n != 0)
        PANIC("requested number of pages (%x) causes an overflow");

    static paddr_t next_paddr = (paddr_t) __free_ram;
    paddr_t paddr = next_paddr;
    next_paddr += n * PAGE_SIZE;

    if (next_paddr > (paddr_t) __free_ram_end)
        PANIC("out of memory");

    // allocating newly allocated pages to 0 ensures consistency and security
    memset((void *) paddr, 0, n * PAGE_SIZE);

    return paddr;
}

/*
 * --------------------------------------------------------------------------------
 * PROCESS MANAGEMENT
 * --------------------------------------------------------------------------------
 */

extern char __kernel_base[];
struct proc *init_proc(const void *image, size_t image_size)
{
    // TODO: revisit and ensure this is all machine independent
    struct proc *proc = NULL;
    int taken_id;
    for (taken_id = 0; taken_id < PROCS_MAX; taken_id++) {
        if (procs[taken_id].state == UNUSED) {
            proc = &procs[taken_id];
            break;
        }
    }
    if (!proc) PANIC("couldn't init proc, all procs in use");   // XXX: likely want to not panic
    proc->pid = taken_id + 1;
    
    proc = init_proc_ctx(proc, image, image_size);

    // prepare pages
    uint32_t *page_table = (uint32_t *) alloc_pages(1);

    // map kernel pages
    for (paddr_t paddr = (paddr_t) __kernel_base;
            paddr < (paddr_t) __free_ram_end; paddr += PAGE_SIZE)
        map_page_sv32(page_table, paddr, paddr, PAGE_R | PAGE_W | PAGE_X);
    // map user pages
    for (uint32_t off = 0; off < image_size; off += PAGE_SIZE) {
        paddr_t page = alloc_pages(1);

        // data to be copied may be smaller than page size
        size_t remaining = image_size - off;
        size_t copy_size = (PAGE_SIZE <= remaining) ? PAGE_SIZE : remaining;

        // fill and map page
        memcpy((void *) page, image + off, copy_size);
        map_page_sv32(page_table, USER_BASE + off, page,
                PAGE_U | PAGE_R | PAGE_W | PAGE_X);
    }

    proc->page_table = page_table;
    proc->state = RUNNABLE;
    proc->page_table = page_table;

    return proc;
}

void yield(void)
{
    // search for a runnable proc
    // simple round-robin, find next in circle after current proc, then run
    // currently must be called at startup as process 0 is the idle process
    // and we want to move on to an actual process.
    // Essentially, to run actual user processes, need to yield the idle process.
    struct proc *next = idle_proc;
    for (int i = 0; i < PROCS_MAX; i++) {
        struct proc *proc = &procs[(current_proc->pid + i) % PROCS_MAX];
        if (proc->state == RUNNABLE && proc->pid > 0) {
            next = proc;
            break;
        }
    }

    if (next == current_proc) return;   // circled around and selected self

    save_kern_state(next);

    // switch
    struct proc *prev = current_proc;
    current_proc = next;
    switch_context(&prev->sp, &next->sp);
}

/*
 * ----------------------------------------------------------------------------------
 * VIRTIO DISK I/O
 * ----------------------------------------------------------------------------------
 */
uint32_t virtio_reg_read32(unsigned offset)
{
    return *((volatile uint32_t *) (VIRTIO_BLK_PADDR + offset));
}
uint64_t virtio_reg_read64(unsigned offset)
{
    return *((volatile uint64_t *) (VIRTIO_BLK_PADDR + offset));
}
void virtio_reg_write32(unsigned offset, uint32_t value)
{
    *((volatile uint32_t *) (VIRTIO_BLK_PADDR + offset)) = value;
}
void virtio_reg_fetch_and_or32(unsigned offset, uint32_t value)
{
    virtio_reg_write32(offset, virtio_reg_read32(offset) | value);
}

struct virtio_virtq *blk_request_vq;
struct virtio_blk_req *blk_req;
paddr_t blk_req_paddr;
uint64_t blk_capacity;

struct virtio_virtq *virtq_init(unsigned index)
{
    // Virtual queue is configured as follows:
    // 1. Select the queue writing its index (first queue is 0) to QueueSel.
    // 2. Check if the queue is not already in use: Read QueuePFN, expecting a returned value of zero (0x0).
    // 3. Read maximum queue size (number of elements) from QueueNumMax.
    //  If the returned value is zero (0x0) the queue is not available.
    // 4. Allocate and zero the queue pages in contiguous virtual memory,
    //  aligning the Used Ring to an optimal boundary (usually page size).
    //  The driver should choose a queue size smaller than or equal to QueueNumMax.
    // 5. Notify the device about the queue size by writing the size to QueueNum.
    // 6. Notify the device about the used alignment by writing its value in bytes
    //  to QueueAlign.
    // 7. Write the physical number of the first page of the queue to the QueuePFN register.
    paddr_t virtq_paddr = alloc_pages(align_up(sizeof(struct virtio_virtq), PAGE_SIZE) / PAGE_SIZE);
    struct virtio_virtq *vq = (struct virtio_virtq *) virtq_paddr;
    vq->queue_index = index;
    vq->used_index = (volatile uint16_t *) &vq->used.index;
    // 1.
    virtio_reg_write32(VIRTIO_REG_QUEUE_SEL, index);
    // 5.
    virtio_reg_write32(VIRTIO_REG_QUEUE_NUM, VIRTQ_ENTRY_NUM);
    // 6.
    virtio_reg_write32(VIRTIO_REG_QUEUE_ALIGN, 0);
    // 7.
    virtio_reg_write32(VIRTIO_REG_QUEUE_PFN, virtq_paddr);
    return vq;
}

void virtio_blk_init(void)
{
    // initialization defined in spec:
    // https://docs.oasis-open.org/virtio/virtio/v1.1/csprd01/virtio-v1.1-csprd01.html#x1-910003
    // 1. Reset the device.
    // 2. Set the ACK bit- OS recognizes device.
    // 3. Set DRIVER bit- guest OS knows how to drive the device.
    // 4. Read device feature bits, write subset of feature bits understood by the OS
    //  and driver to the device.
    //  During this step, the driver MAY read (but MOST NOT write)
    //  the device-specific configuration fields to check that it 
    //  can support the device before accepting it.
    // 5. Set the FEATURES_OK status bit.
    //  The driver MUST NOT accept new feature bits after this step.
    // 6. Re-read devie status to ensure the FEATURES_OK bit is still set:
    //  otherwise, the device does not support our subset of features
    //  and the device is unstable.
    // 7. Perform device-specific setup,
    //   including discovery of virtqueues for the device,
    //   optional per-bus setup, reading and possibly writing
    //   the device's virtio configuration space, and population of virtqueues.
    // 8. Set the DRIVER_OK status bit. At this point, the device is "live".
    if (virtio_reg_read32(VIRTIO_REG_MAGIC) != 0x74726976)
        PANIC("virtio: invalid magic value");
    if (virtio_reg_read32(VIRTIO_REG_VERSION) != 1)
        PANIC("virtio: invalid version");
    if (virtio_reg_read32(VIRTIO_REG_DEVICE_ID) != VIRTIO_DEVICE_BLK)
        PANIC("virtio: invalid device id");

    // 1.
    virtio_reg_write32(VIRTIO_REG_DEVICE_STATUS, 0);
    // 2.
    virtio_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_ACK);
    // 3.
    virtio_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_DRIVER);
    // 5.
    virtio_reg_fetch_and_or32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_FEAT_OK);
    // 7.
    blk_request_vq = virtq_init(0);
    // 8.
    virtio_reg_write32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_DRIVER_OK);

    // get disk capacity
    blk_capacity = virtio_reg_read64(VIRTIO_REG_DEVICE_CONFIG + 0) * SECTOR_SIZE;
    printf("virtio-blk: capacity is %d bytes\n", blk_capacity);

    // allocate region to store requests to device
    blk_req_paddr = alloc_pages(align_up(sizeof(*blk_req), PAGE_SIZE) / PAGE_SIZE);
    blk_req = (struct virtio_blk_req *) blk_req_paddr;
}

void virtq_kick(struct virtio_virtq *vq, int desc_index)
{
    vq->avail.ring[vq->avail.index % VIRTQ_ENTRY_NUM] = desc_index;
    vq->avail.index++;
    __sync_synchronize();
    virtio_reg_write32(VIRTIO_REG_QUEUE_NOTIFY, vq->queue_index);
    vq->last_used_index++;
}

bool virtq_is_busy(struct virtio_virtq *vq)
{
    return vq->last_used_index != *vq->used_index;
}

void read_write_disk(void *buf, unsigned sector, bool is_write)
{
    // 1. Construct a request in blk_req. Specify the sector number, and r/w.
    // 2. Construct a descriptor chain pointing to each area of blk_req.
    // 3. Add the index of the head descriptor of the descriptor chain to the Available Ring.
    // 4. Notify the device that there is a new pending request.
    // 5. Wait until the device finished processing.
    // 6. Check the response from the device.

    if (sector >= blk_capacity / SECTOR_SIZE) {
        printf("virtio: tried to read/write sector=%d, but capacity is %d\n",
              sector, blk_capacity / SECTOR_SIZE);
        return;
    }

    // Construct the request according to the virtio-blk specification.
    blk_req->sector = sector;
    blk_req->type = is_write ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
    if (is_write)
        memcpy(blk_req->data, buf, SECTOR_SIZE);

    // Construct the virtqueue descriptors (using 3 descriptors).
    struct virtio_virtq *vq = blk_request_vq;
    vq->descs[0].addr = blk_req_paddr;
    vq->descs[0].len = sizeof(uint32_t) * 2 + sizeof(uint64_t);
    vq->descs[0].flags = VIRTQ_DESC_F_NEXT;
    vq->descs[0].next = 1;

    vq->descs[1].addr = blk_req_paddr + offsetof(struct virtio_blk_req, data);
    vq->descs[1].len = SECTOR_SIZE;
    vq->descs[1].flags = VIRTQ_DESC_F_NEXT | (is_write ? 0 : VIRTQ_DESC_F_WRITE);
    vq->descs[1].next = 2;

    vq->descs[2].addr = blk_req_paddr + offsetof(struct virtio_blk_req, status);
    vq->descs[2].len = sizeof(uint8_t);
    vq->descs[2].flags = VIRTQ_DESC_F_WRITE;

    // Notify the device that there is a new request.
    virtq_kick(vq, 0);

    // Wait until the device finishes processing.
    while (virtq_is_busy(vq))
        ;

    // virtio-blk: If a non-zero value is returned, it's an error.
    if (blk_req->status != 0) {
        printf("virtio: warn: failed to read/write sector=%d status=%d\n",
               sector, blk_req->status);
        return;
    }

    // For read operations, copy the data into the buffer.
    if (!is_write)
        memcpy(buf, blk_req->data, SECTOR_SIZE);
}

/*
 * ----------------------------------------------------------------------------------
 * FILE SYSTEM
 * ----------------------------------------------------------------------------------
 */

struct file files[FILES_MAX];
uint8_t disk[DISK_MAX_SIZE];

int oct2int(char *oct, int len)
{
    int dec = 0;
    for (int i = 0; i < len; i++) {
        if (oct[i] < '0' || oct[i] > '7')
            break;

        dec = dec * 8 + (oct[i] - '0');
    }
    return dec;
}

void fs_init(void)
{
    /*
     * Initializes by directly loading each file in archive into memory.
     */
    for (unsigned sector = 0; sector < sizeof(disk) / SECTOR_SIZE; sector++)
        read_write_disk(&disk[sector * SECTOR_SIZE], sector, false);

    unsigned off = 0;
    for (int i = 0; i < FILES_MAX; i++) {
        struct tar_header *header = (struct tar_header *) &disk[off];
        if (header->name[0] == '\0')
            break;

        if (strcmp(header->magic, "ustar") != 0)
            PANIC("invalid tar header: magic=\"%s\"", header->magic);

        int filesz = oct2int(header->size, sizeof(header->size));

        struct file *file = &files[i];
        file->in_use = true;
        strcpy(file->name, header->name);
        memcpy(file->data, header->data, filesz);
        file->size = filesz;
        printf("file: %s, size=%d\n", file->name, file->size);

        off += align_up(sizeof(struct tar_header) + filesz, SECTOR_SIZE);
    }
}

void fs_flush(void)
{
    // copy file contents to disk buffer
    memset(disk, 0, sizeof(disk));
    unsigned off = 0;
    for (int file_i = 0; file_i < FILES_MAX; file_i++) {
        struct file *file = &files[file_i];
        if (!file->in_use)
            continue;

        struct tar_header *header = (struct tar_header *) &disk[off];
        memset(header, 0, sizeof(*header));
        strcpy(header->name, file->name);
        strcpy(header->mode, "000644");
        strcpy(header->magic, "ustar");
        strcpy(header->version, "00");
        header->type = '0';

        // turn into octal string
        int filesz = file->size;
        for (int i = sizeof(header->size); i > 0; i--) {
            header->size[i - 1] = (filesz % 8) + '0';
            filesz /= 8;
        }

        // calculate checksum
        int checksum = ' ' * sizeof(header->checksum);
        for (unsigned i = 0; i < sizeof(struct tar_header); i++)
            checksum += (unsigned char) disk[off + i];
        for (int i = 5; i >= 0; i--) {
            header->checksum[i] = (checksum % 8) + '0';
            checksum /= 8;
        }

        // copy file data
        memcpy(header->data, file->data, file->size);
        off += align_up(sizeof(struct tar_header) + file->size, SECTOR_SIZE);
    }

    // write `disk` buffer into virtio-blk
    for (unsigned sector = 0; sector < sizeof(disk) / SECTOR_SIZE; sector++)
        read_write_disk(&disk[sector * SECTOR_SIZE], sector, true);

    printf("wrote %d ytes to disk\n", sizeof(disk));
}

struct file *fs_lookup(const char *filename)
{
    for (int i = 0; i < FILES_MAX; i++) {
        struct file *file = &files[i];
        if (!strcmp(file->name, filename))
            return file;
    }

    return NULL;
}

//  ---------------------------
// testing functions
void delay(void)
{
    for (int i = 0; i < 300000000; i++)
        __asm__ __volatile__("nop");
}

void proc_a_entry(void)
{
    printf("starting proc A\n");
    for (int i = 0; i < 300; i++) {
        putchar('A');
        yield();
        delay();
    }
}

void proc_b_entry(void)
{
    printf("starting process B\n");
    for (int i = 0; i < 300; i++) {
        putchar('B');
        yield();
        delay();
    }
}

//  ---------------------------

