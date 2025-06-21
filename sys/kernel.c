#include "kernel.h"
#include "../common.h"
#include "riscv.h"

extern uint8_t __bss[], __bss_end[];	// taken from kernel.ld

/*
void sbi_console_putstr(char *s)
{
     * Puts a null-terminated string.
     * NOT a part of the OpenSBI spec afaict, but a nice encoding.
    char *current = s;
    while (*current)
    {
        putchar(*current);
        current++;
    }
}
*/

void kernel_main(void)
{
	memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);  // set bss to 0 as a sanity check

    char *hello = "\r\nHello, World!\r\n";
    printf(hello);
    printf("guh, %s is certainly a string...\n", "this");

	for (;;)
        __asm__ __volatile__("wfi");    // FIXME: depends on riscv
}

