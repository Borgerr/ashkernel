#include "kernel.h"
#include "common.h"

extern uint8_t __bss[], __bss_end[], __stack_top[];	// taken from kernel.ld

struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4,
		long arg5, long fid, long eid)
{
    /*
     * S-Mode (kernel) to M-Mode (OpenSBI), OpenSBI invokes processing handler,
     * and switches back to kernel mode.
     */

    // asks compiler to place values on rhs in specified registers, variables lhs
    // reference: https://git.musl-libc.org/cgit/musl/tree/arch/riscv64/syscall_arch.h
	register long a0 __asm__("a0") = arg0;
	register long a1 __asm__("a1") = arg1;
	register long a2 __asm__("a2") = arg2;
	register long a3 __asm__("a3") = arg3;
	register long a4 __asm__("a4") = arg4;
	register long a5 __asm__("a5") = arg5;
	register long a6 __asm__("a6") = fid;
	register long a7 __asm__("a7") = eid;

    __asm__ __volatile__("ecall"
            : "=r"(a0), "=r"(a1)
            : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5),
              "r"(a6), "r"(a7)
            : "memory");
    return (struct sbiret){.error = a0, .value = a1};
}

void *memset(void *buf, char c, size_t size)
{
	/*
	 * set membuf at `void *buf` to `char c`
	 * buf assumed to be of size `size`
	 */
	uint8_t *current = (uint8_t *) buf;
	for (size_t i = 0; i < size; i++)
		*current++ = c; // XXX: maybe an off by 1 error idk
	return buf;
}

void putchar(char ch)
{
    /*
     * long sbi_console_putchar(int ch);
     * Puts a char to the screen.
     * SBI call blocks if remaining pending characters, or receiving terminal isn't ready to receive.
     *
     * Follows SBI calling conventions
     * EID 0x01
     */
    sbi_call(ch, 0, 0, 0, 0, 0, 0, 1); // EID = 0x01, arg 0 is ch
}

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
	memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);

    char *hello = "\r\nHello, World!\r\n";
    printf(hello);
    printf("guh, %s is certainly a string...", "this");

	for (;;)
        __asm__ __volatile__("wfi");
}

// XXX: specific to RISC-V
// with potential hardware ports, move.
__attribute__((section(".text.boot")))
__attribute__((naked))
void boot(void)
{
	/*
	 * kernel.ld points to here as start (section attribute)
	 * __attribute__((naked)) removes prelude and epilog for function
	 */
	__asm__ __volatile__(
		"mv sp, %[stack_top]\n"	// set stack pointer
		"j kernel_main\n"	// jump to kernel main
		:
		: [stack_top] "r" (__stack_top)	// %[stack_top] in asm == __stack_top in C
	);
}

