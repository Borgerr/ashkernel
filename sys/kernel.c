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

void kernel_main(void)
{
	memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);  // set bss to 0 as a sanity check
    WRITE_CSR(stvec, (uint32_t) kernel_entry);

    idle_proc = init_proc((uint32_t) NULL);
    idle_proc->pid = 0;
    current_proc = idle_proc;   // boot process context saved, can return to later

    proc_a = init_proc((uint32_t) proc_a_entry);
    proc_b = init_proc((uint32_t) proc_b_entry);

    yield();
    PANIC("system is idle!");

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
static paddr_t next_paddr = (paddr_t) __free_ram;

paddr_t alloc_pages(uint32_t n)
{
    /*
     * Allocates `n` pages
     * Simple bump allocator for now.
     * TODO: make more sophisticated-- we want eventual high memory utilization
     * and ways to free pages.
     * RISC-V also likely has some MMU utility we can wrangle here for S-Mode vs M-Mode.
     * Also always panics on invalid requests-- may want to propagate an "invalid request" sort of signal instead for
     * a robust system.
     *
     * XXX: should this go in riscv.c?
     */
    if (n * PAGE_SIZE == 0 && n != 0)
        PANIC("requested number of pages (%x) causes an overflow");

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
struct proc *init_proc(uint32_t pc) // TODO: change to not rely on program counter
{
    struct proc *proc = NULL;
    int taken_id;
    for (taken_id = 0; taken_id < PROCS_MAX; taken_id++) {
        if (procs[taken_id].state == PROC_UNUSED) {
            proc = &procs[taken_id];
            break;
        }
    }
    if (!proc) PANIC("couldn't init proc, all procs in use");   // XXX: likely want to not panic
    
    return init_proc_ctx(pc, proc, taken_id);
}

void yield(void)
{
    // search for a runnable proc
    // simple round-robin, find next in circle after current proc, then run
    struct proc *next = idle_proc;
    for (int i = 0; i < PROCS_MAX; i++) {
        struct proc *proc = &procs[(current_proc->pid + i) % PROCS_MAX];
        if (proc->state == PROC_RUNNABLE && proc->pid > 0) {
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

