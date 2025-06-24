#include "kernel.h"
#include "../common.h"
#include "riscv.h"  // TODO: change depending on target

extern uint8_t __bss[], __bss_end[];	// taken from kernel.ld

struct proc procs[PROCS_MAX];

void delay(void)
{
    for (int i = 0; i < 30000000; i++)
        __asm__ __volatile__("nop");
}
struct proc *proc_a;
struct proc *proc_b;

void proc_a_func(void)
{
    for (int i = 0; i < 300; i++) {
        putchar('A');
        switch_context(&proc_a->sp, &proc_b->sp);
        delay();
    }
}

void proc_b_func(void)
{
    for (int i = 0; i < 300; i++) {
        putchar('B');
        switch_context(&proc_b->sp, &proc_a->sp);
        delay();
    }
}

void kernel_main(void)
{
	memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);  // set bss to 0 as a sanity check
    WRITE_CSR(stvec, (uint32_t) kernel_entry);
    //__asm__ __volatile__("unimp");    // uncomment to test exception handling
    paddr_t paddr0 = alloc_pages(2);    // free_ram += 2 * 4096
    paddr_t paddr1 = alloc_pages(1);    // free_ram += 1 * 4096
    printf("paddr0: %x\n", paddr0);
    printf("paddr1: %x\n", paddr1);

    paddr_t paddr_rest = alloc_pages(64); // should panic
    printf("paddr_rest, %x, was erroneously allowed to be evaluated\n", paddr_rest);

    proc_a = init_proc((uint32_t) proc_a_func);
    proc_b = init_proc((uint32_t) proc_b_func);

    proc_a_func();

	for (;;)
        __asm__ __volatile__("wfi");    // FIXME: depends on riscv
}

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
     */
    paddr_t paddr = next_paddr;
    next_paddr += n * PAGE_SIZE;

    printf("next_paddr: %x\n", next_paddr);

    if (next_paddr > (paddr_t) __free_ram_end)
        PANIC("out of memory");

    // allocating newly allocated pages to 0 ensures consistency and security
    memset((void *) paddr, 0, n * PAGE_SIZE);
    return paddr;
}

