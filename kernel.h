#pragma once

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

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

void *memset(void *buf, char c, size_t size);
struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4,
		long arg5, long fid, long eid);

