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

void handle_syscall(struct trap_frame *f)
{
    switch (f->a3) {    // syscall ID
        case SYS_PUTCHAR:
            putchar(f->a0);     // actual arg (XXX: definitely didn't check ;^) make this safe!)
            break;
        default:
            PANIC("unexpected syscall a3=%x\n", f->a3);
            break;
    }
}

void handle_trap(struct trap_frame *f)
{
    // TODO: handle different exceptions
    uint32_t scause = READ_CSR(scause);
    uint32_t stval = READ_CSR(stval);
    uint32_t user_pc = READ_CSR(sepc);

    //printf("\n\nscause: %x, SCAUSE_ECALL: %x\n\n", scause, SCAUSE_ECALL);

    switch (scause) {
        case SCAUSE_ECALL:
            //printf("SCAUSE_ECALL trap\n");
            handle_syscall(f);
            user_pc += 4;       // move past syscall invocation
            break;
        case SCAUSE_LFALT:
            PANIC("PAGE LOAD FAULT!!! paging is not yet implemented.\noffending instr at addr %x tried accessing %x\n\n", user_pc, stval);
            break;
        case SCAUSE_SFALT:
            // TODO: come back to this when we plan on supporting paging
            PANIC("PAGE STORE FAULT!!! paging is not yet implemented.\noffending instr at addr %x tried accessing %x\n\n", user_pc, stval);
            break;
        default:
            PANIC("\r\nUNRECOGNIZED EXCEPTION: scause=%x, stval=%x, sepc=%x\n", scause, stval, user_pc);
            break;
    }

    WRITE_CSR(sepc, user_pc);
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
     *
     * Requires 31 available words on a kernel stack.
     * Does not support nested interrupt handling.
     */
    __asm__ __volatile__(
        "csrrw sp, sscratch, sp\n"  // get kernel stack; swaps in one instruction

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

        // stack pointer saved
        "csrr a0, sscratch\n"
        "sw a0, 4 * 30(sp)\n"

        // change kernel stack
        "addi a0, sp, 4 * 31\n"
        "csrw sscratch, a0\n"

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
 * PROCESS MANAGEMENT
 * --------------------------------------------------------------------------------
 */

__attribute__((naked))
void switch_context(uint32_t *prev_sp, uint32_t *next_sp)
{
    /*
     * Really dumb context switching mechanism where we save user's state on their kern_stack
     * and then switch to another stack and continue execution.
     * TODO: definitely want to change this to be more than swapping a stack pointer
     */
    __asm__ __volatile__(
        // Save callee-saved registers onto the current process's stack.
        "addi sp, sp, -13 * 4\n" // Allocate stack space for 13 4-byte registers
        "sw ra,  0  * 4(sp)\n"   // Save callee-saved registers only
        "sw s0,  1  * 4(sp)\n"
        "sw s1,  2  * 4(sp)\n"
        "sw s2,  3  * 4(sp)\n"
        "sw s3,  4  * 4(sp)\n"
        "sw s4,  5  * 4(sp)\n"
        "sw s5,  6  * 4(sp)\n"
        "sw s6,  7  * 4(sp)\n"
        "sw s7,  8  * 4(sp)\n"
        "sw s8,  9  * 4(sp)\n"
        "sw s9,  10 * 4(sp)\n"
        "sw s10, 11 * 4(sp)\n"
        "sw s11, 12 * 4(sp)\n"

        // Switch the stack pointer.
        // REMEMBER: prev_sp and next_sp on stack as part of function invocation
        "sw sp, (a0)\n"         // *prev_sp = sp;
        "lw sp, (a1)\n"         // Switch stack pointer (sp) here

        // Restore callee-saved registers from the next process's stack.
        "lw ra,  0  * 4(sp)\n"  // Restore callee-saved registers only
        "lw s0,  1  * 4(sp)\n"
        "lw s1,  2  * 4(sp)\n"
        "lw s2,  3  * 4(sp)\n"
        "lw s3,  4  * 4(sp)\n"
        "lw s4,  5  * 4(sp)\n"
        "lw s5,  6  * 4(sp)\n"
        "lw s6,  7  * 4(sp)\n"
        "lw s7,  8  * 4(sp)\n"
        "lw s8,  9  * 4(sp)\n"
        "lw s9,  10 * 4(sp)\n"
        "lw s10, 11 * 4(sp)\n"
        "lw s11, 12 * 4(sp)\n"
        "addi sp, sp, 13 * 4\n"  // We've popped 13 4-byte registers from the stack
        "ret\n"
    );
}

extern struct proc procs[];
extern char __kernel_base[], __free_ram_end[];

__attribute__((naked))
void user_entry(void)
{
    __asm__ __volatile__(
        "csrw sepc, %[sepc]         \n" // sepc sets pc when switching to U-Mode
        "csrw sstatus, %[sstatus]   \n" // hardware interrupts enabled (SSTATUS_SPIE bit)
        "sret                       \n"
        :
        : [sepc] "r" (USER_BASE),       // TODO: change this to be a parameter or something
          [sstatus] "r" (SSTATUS_SPIE)
    );
}

__attribute__((always_inline))
struct proc *init_proc_ctx(struct proc *proc, const void *image, size_t image_size)
{
    // context stored on kernel stack
    uint32_t *sp = (uint32_t *) &proc->kern_stack[sizeof(proc->kern_stack)];    // start at top of stack
    for (int i = 0; i < 12; i++)    // initialize s11, s10, s9, s8, etc to 0
        *--sp = 0;
    *--sp = (uint32_t) (uint32_t) user_entry;  // ra (returns to user_entry function in kernel)

    
    proc->sp = (uint32_t) sp;

    return proc;
}

/*
 * --------------------------------------------------------------------------------
 * SATP_V32 VIRTUAL MEMORY
 * --------------------------------------------------------------------------------
 */

void map_page_sv32(uint32_t *table1, vaddr_t vaddr, paddr_t paddr, uint32_t flags)
{
    if (!is_aligned(vaddr, PAGE_SIZE))
        PANIC("unaligned vaddr %x", vaddr);

    if (!is_aligned(paddr, PAGE_SIZE))
        PANIC("unaligned paddr %x", paddr);

    uint32_t vpn1 = (vaddr >> 22) & 0x3ff;
    if (!(table1[vpn1] & PAGE_V)) {
        // Create the 1st level page table if it doesn't exist.
        uint32_t pt_paddr = alloc_pages(1);
        table1[vpn1] = ((pt_paddr / PAGE_SIZE) << 10) | PAGE_V;
    }

    // Set the 2nd level page table entry to map the physical page.
    uint32_t vpn0 = (vaddr >> 12) & 0x3ff;
    uint32_t *table0 = (uint32_t *) ((table1[vpn1] >> 10) * PAGE_SIZE);
    table0[vpn0] = ((paddr / PAGE_SIZE) << 10) | flags | PAGE_V;
}

__attribute__((always_inline))
void save_kern_state(struct proc* next)
{
    __asm__ __volatile__(
        "sfence.vma\n"
        "csrw satp, %[satp]\n"
        "sfence.vma\n"
        "csrw sscratch, %[sscratch]\n"
        :
        : [satp] "r" (SATP_V32 | ((uint32_t) next->page_table / PAGE_SIZE)),
          [sscratch] "r" ((uint32_t) &next->kern_stack[sizeof(next->kern_stack)])
    );
}

