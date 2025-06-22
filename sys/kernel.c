#include "kernel.h"
#include "../common.h"
#include "riscv.h"

extern uint8_t __bss[], __bss_end[];	// taken from kernel.ld

void kernel_main(void)
{
	memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);  // set bss to 0 as a sanity check
    WRITE_CSR(stvec, (uint32_t) kernel_entry);
    __asm__ __volatile__("unimp");

    char *hello = "\r\nHello, World!\r\n";
    printf(hello);
    printf("guh, %s is certainly a string...\n", "this");

	for (;;)
        __asm__ __volatile__("wfi");    // FIXME: depends on riscv
}

