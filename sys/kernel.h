#pragma once
#include "../common.h"

/*
 * kernel panics!
 * wrapped in a `do while(0)` -- executes once and can more easily be deployed
 * gives source file name (__FILE__) and line (__LINE__)
 * macro allows for multiple arguments with ##__VA_ARGS__
 */
#define PANIC(fmt, ...)                                                         \
    do {                                                                        \
        printf("PANIC: %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);   \
        while (1) {}                                                            \
    } while (0)                                                                 \

/*
 * --------------------------------------------------------------------------------
 * MEMORY MANAGEMENT
 * --------------------------------------------------------------------------------
 */

paddr_t alloc_pages(uint32_t n);    // paddr_t from common.h

/*
 * ----------------------------------------------------------------------------------
 * PROCESS OBJECTS
 * ----------------------------------------------------------------------------------
 */

#define PROCS_MAX       8

struct proc {
    int pid;
    enum proc_state { UNUSED, RUNNABLE, EXITED } state;
    vaddr_t sp;
    uint32_t *page_table;
    uint8_t kern_stack[8192];   // user's GPRs, ret addr, etc, as well as kernel's vars
};

struct proc *init_proc(const void* image, size_t image_size);
void yield(void);

/*
 * ----------------------------------------------------------------------------------
 * USER MODE
 * ----------------------------------------------------------------------------------
 */

#define USER_BASE 0x1000000     // needs to match userspace.ld
#define SSTATUS_SPIE (1 << 5)   // sstatus register SPIE bit controls U-Mode

/*
 * ----------------------------------------------------------------------------------
 * FILE SYSTEM
 * ----------------------------------------------------------------------------------
 */
// XXX: ABSOLUTELY bullshitted.
// Relies on virtio.
// Virtio spec: https://docs.oasis-open.org/virtio/virtio/v1.1/csprd01/virtio-v1.1-csprd01.html
#define SECTOR_SIZE                     512
#define VIRTQ_ENTRY_NUM                 16
#define VIRTIO_DEVICE_BLK               2

#define VIRTIO_BLK_PADDR                0x10001000
#define VIRTIO_REG_MAGIC                0x00
#define VIRTIO_REG_VERSION              0x04
#define VIRTIO_REG_DEVICE_ID            0x08
#define VIRTIO_REG_QUEUE_SEL            0x30
#define VIRTIO_REG_QUEUE_NUM_MAX        0x34
#define VIRTIO_REG_QUEUE_NUM            0x38
#define VIRTIO_REG_QUEUE_ALIGN          0x3c
#define VIRTIO_REG_QUEUE_PFN            0x40
#define VIRTIO_REG_QUEUE_READY          0x44
#define VIRTIO_REG_QUEUE_NOTIFY         0x50
#define VIRTIO_REG_DEVICE_STATUS        0x70
#define VIRTIO_REG_DEVICE_CONFIG        0x100

#define VIRTIO_STATUS_ACK       1
#define VIRTIO_STATUS_DRIVER    2
#define VIRTIO_STATUS_DRIVER_OK 4
#define VIRTIO_STATUS_FEAT_OK   8

#define VIRTQ_DESC_F_NEXT               1
#define VIRTQ_DESC_F_WRITE              2
#define VIRTQ_AVAIL_F_NO_INTERRUPT      1

#define VIRTIO_BLK_T_IN         0
#define VIRTIO_BLK_T_OUT        1

// descriptor area
struct virtq_desc {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} __attribute__((packed));

// available ring
struct virtq_avail {
    uint16_t flags;
    uint16_t index;
    uint16_t ring[VIRTQ_ENTRY_NUM];
} __attribute__((packed));

// used ring entry
struct virtq_used_elem {
    uint32_t id;
    uint32_t len;
} __attribute__((packed));

// used ring
struct virtq_used {
    uint16_t flags;
    uint16_t index;
    struct virtq_used_elem ring[VIRTQ_ENTRY_NUM];
} __attribute__((packed));

// virtqueue
struct virtio_virtq {
    struct virtq_desc descs[VIRTQ_ENTRY_NUM];
    struct virtq_avail avail;
    struct virtq_used used __attribute__((aligned(PAGE_SIZE)));
    int queue_index;
    volatile uint16_t *used_index;
    uint16_t last_used_index;
} __attribute__((packed));

struct virtio_blk_req {
    uint32_t type;
    uint32_t reserved;
    uint64_t sector;
    uint8_t data[512];
    uint8_t status;
} __attribute__((packed));

