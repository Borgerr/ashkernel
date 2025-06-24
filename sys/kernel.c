#include "kernel.h"
#include "../common.h"
#include "riscv.h"  // TODO: change depending on target

extern uint8_t __bss[], __bss_end[];	// taken from kernel.ld

struct proc procs[PROCS_MAX];

void kernel_main(void)
{
	memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);  // set bss to 0 as a sanity check
    WRITE_CSR(stvec, (uint32_t) kernel_entry);
    //__asm__ __volatile__("unimp");    // uncomment to test exception handling
    paddr_t paddr0 = alloc_pages(2);    // free_ram += 2 * 4096
    paddr_t paddr1 = alloc_pages(1);    // free_ram += 1 * 4096
    printf("paddr0: %x\n", paddr0);
    printf("paddr1: %x\n", paddr1);

    paddr_t paddr_rest = alloc_pages(64 * 1024 * 1024); // should panic

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

    return init_proc_ctx(pc, proc, i);
}

