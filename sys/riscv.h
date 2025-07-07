#pragma once

#include "../common.h"
#include "kernel.h"

/*
 * --------------------------------------------------------------------------------
 * OpenSBI
 * --------------------------------------------------------------------------------
 */

/*
 * ---- From RISC-V SBI 3.0-rc8, chapter 3
 * All SBI functions share a single binary encoding,
 * which facilitates the mixing of SBI extensions.
 * The SBI specification follows the below calling convention.
 *
 * - An `ECALL` is used as the control transfer instruciton between the supervisor and the SEE.
 * - `a7` encodes the SBI extension ID (EID).
 * - `a6` encodes the SBI funciton ID (FID) for a given extension ID encoded in a7 for any SBI extension defined in or after SBI v0.2.
 * - `a0` through `a5` contain the arguments for the SBI function call. Registers that are not defined in the SBI function call are not reserved.
 * - All register except `a0` & `a1` must be preserved across an SBI call by the callee.
 * - SBI functions must return a pair of values in `a0` and `a1`, with `a0` returning an error code. This is analagous to returning the C structure:
 */
struct sbiret
{
	long error;
	union {
		long value;
		unsigned long uvalue;
	};
};

struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4,
		long arg5, long fid, long eid);

/*
 * --------------------------------------------------------------------------------
 * EXCEPTION HANDLING
 * --------------------------------------------------------------------------------
 */

struct trap_frame {
    uint32_t ra;
    uint32_t gp;
    uint32_t tp;
    uint32_t t0;
    uint32_t t1;
    uint32_t t2;
    uint32_t t3;
    uint32_t t4;
    uint32_t t5;
    uint32_t t6;
    uint32_t a0;
    uint32_t a1;
    uint32_t a2;
    uint32_t a3;
    uint32_t a4;
    uint32_t a5;
    uint32_t a6;
    uint32_t a7;
    uint32_t s0;
    uint32_t s1;
    uint32_t s2;
    uint32_t s3;
    uint32_t s4;
    uint32_t s5;
    uint32_t s6;
    uint32_t s7;
    uint32_t s8;
    uint32_t s9;
    uint32_t s10;
    uint32_t s11;
    uint32_t sp;
} __attribute__((packed));

#define READ_CSR(reg)                                           \
    ({                                                          \
        unsigned long __tmp;                                    \
        __asm__ __volatile__("csrr %0, " #reg : "=r"(__tmp));   \
        __tmp;                                                  \
    })                                                          \

#define WRITE_CSR(reg, value)                                   \
    do {                                                        \
        uint32_t __tmp = (value);                               \
        __asm__ __volatile__("csrw " #reg ", %0" ::"r"(__tmp)); \
    } while (0)                                                 \

void kernel_entry(void);

/*
 * --------------------------------------------------------------------------------
 * PROCESS MANAGEMENT
 * --------------------------------------------------------------------------------
 */

void switch_context(uint32_t *prev_sp, uint32_t *next_sp);

struct proc *init_proc_ctx(struct proc *proc, const void *image, size_t image_size);

/*
 * --------------------------------------------------------------------------------
 * SATP_V32 VIRTUAL MEMORY
 * --------------------------------------------------------------------------------
 */

#define SATP_V32    (1u << 31)
#define PAGE_V      (1 << 0)    // valid
#define PAGE_R      (1 << 1)    // readable
#define PAGE_W      (1 << 2)    // writable
#define PAGE_X      (1 << 3)    // executable
#define PAGE_U      (1 << 4)    // U-Mode accessible

void map_page_sv32(uint32_t *table1, vaddr_t vaddr, paddr_t paddr, uint32_t flags);

void save_kern_state(struct proc *next);

