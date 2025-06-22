#include "kernel.h"
#include "../common.h"
#include "riscv.h"

/*
 * --------------------------------------------------------------------------------
 * OpenSBI
 * --------------------------------------------------------------------------------
 */

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

extern uint8_t __stack_top[];

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
 * --------------------------------------------------------------------------------
 * EXCEPTION HANDLING
 * --------------------------------------------------------------------------------
 */

void handle_trap(struct trap_frame *f)
{
    // TODO: handle different exceptions
    uint32_t scause = READ_CSR(scause);
    uint32_t stval = READ_CSR(stval);
    uint32_t user_pc = READ_CSR(sepc);

    PANIC("\r\nUNHANDLED EXCEPTION: scause=%x, stval=%x, sepc=%x\n", scause, stval, user_pc);
}

__attribute__((naked))
__attribute__((aligned(4))) // stvec bit 0 for flags
void kernel_entry(void)
{
    /*
     * `stvec` points here -- main exception handler
     * CSR regs will allow for further branching, but this saves state for proc
     *
     * MUST keep `struct trap_frame` with same order of registers
     * and must keep stack ptr set in a0 before calling `handle_trap`
     * to keep a valid trap_frame
     */
    __asm__ __volatile__(
        "csrw sscratch, sp\n"
        "addi sp, sp, -4 * 31\n"
        "sw ra,  4 * 0(sp)\n"
        "sw gp,  4 * 1(sp)\n"
        "sw tp,  4 * 2(sp)\n"
        "sw t0,  4 * 3(sp)\n"
        "sw t1,  4 * 4(sp)\n"
        "sw t2,  4 * 5(sp)\n"
        "sw t3,  4 * 6(sp)\n"
        "sw t4,  4 * 7(sp)\n"
        "sw t5,  4 * 8(sp)\n"
        "sw t6,  4 * 9(sp)\n"
        "sw a0,  4 * 10(sp)\n"
        "sw a1,  4 * 11(sp)\n"
        "sw a2,  4 * 12(sp)\n"
        "sw a3,  4 * 13(sp)\n"
        "sw a4,  4 * 14(sp)\n"
        "sw a5,  4 * 15(sp)\n"
        "sw a6,  4 * 16(sp)\n"
        "sw a7,  4 * 17(sp)\n"
        "sw s0,  4 * 18(sp)\n"
        "sw s1,  4 * 19(sp)\n"
        "sw s2,  4 * 20(sp)\n"
        "sw s3,  4 * 21(sp)\n"
        "sw s4,  4 * 22(sp)\n"
        "sw s5,  4 * 23(sp)\n"
        "sw s6,  4 * 24(sp)\n"
        "sw s7,  4 * 25(sp)\n"
        "sw s8,  4 * 26(sp)\n"
        "sw s9,  4 * 27(sp)\n"
        "sw s10, 4 * 28(sp)\n"
        "sw s11, 4 * 29(sp)\n"

        "csrr a0, sscratch\n"
        "sw a0, 4 * 30(sp)\n"

        "mv a0, sp\n"
        "call handle_trap\n"

        "lw ra,  4 * 0(sp)\n"
        "lw gp,  4 * 1(sp)\n"
        "lw tp,  4 * 2(sp)\n"
        "lw t0,  4 * 3(sp)\n"
        "lw t1,  4 * 4(sp)\n"
        "lw t2,  4 * 5(sp)\n"
        "lw t3,  4 * 6(sp)\n"
        "lw t4,  4 * 7(sp)\n"
        "lw t5,  4 * 8(sp)\n"
        "lw t6,  4 * 9(sp)\n"
        "lw a0,  4 * 10(sp)\n"
        "lw a1,  4 * 11(sp)\n"
        "lw a2,  4 * 12(sp)\n"
        "lw a3,  4 * 13(sp)\n"
        "lw a4,  4 * 14(sp)\n"
        "lw a5,  4 * 15(sp)\n"
        "lw a6,  4 * 16(sp)\n"
        "lw a7,  4 * 17(sp)\n"
        "lw s0,  4 * 18(sp)\n"
        "lw s1,  4 * 19(sp)\n"
        "lw s2,  4 * 20(sp)\n"
        "lw s3,  4 * 21(sp)\n"
        "lw s4,  4 * 22(sp)\n"
        "lw s5,  4 * 23(sp)\n"
        "lw s6,  4 * 24(sp)\n"
        "lw s7,  4 * 25(sp)\n"
        "lw s8,  4 * 26(sp)\n"
        "lw s9,  4 * 27(sp)\n"
        "lw s10, 4 * 28(sp)\n"
        "lw s11, 4 * 29(sp)\n"
        "lw sp,  4 * 30(sp)\n"
        "sret\n"
    );
}

/*
 * --------------------------------------------------------------------------------
 * MEMORY MANAGEMENT
 * --------------------------------------------------------------------------------
 */
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

    if (next_paddr > (paddr_t) __free_ram_end)
        PANIC("out of memory");

    // allocating newly allocated pages to 0 ensures consistency and security
    memset((void *) paddr, 0, n * PAGE_SIZE);
    return paddr;
}

