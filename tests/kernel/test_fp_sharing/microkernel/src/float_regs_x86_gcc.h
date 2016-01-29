/* Intel x86 GCC specific floating point register macros */

/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _FLOAT_REGS_X86_GCC_H
#define _FLOAT_REGS_X86_GCC_H

#if !defined(__GNUC__) || !defined(CONFIG_ISA_IA32)
#error test_asm_inline_gcc.h goes only with x86 GCC
#endif

#include <toolchain.h>
#include "float_context.h"

/**
 *
 * @brief Load all floating point registers
 *
 * This function loads ALL floating point registers from the memory buffer
 * specified by <pFromBuffer>. It is expected that a subsequent call to
 * _StoreAllFloatRegisters() will be issued to dump the floating point registers
 * to memory.
 *
 * The format/organization of the FP_REG_SET structure is not important; the
 * generic C test code (main.c and fiber.c) merely treat the FP_REG_SET
 * (and FP_NONVOLATILE_REG_SET) as an array of bytes.
 *
 * The only requirement is that the arch specific implementations of
 * _LoadAllFloatRegisters(), _StoreAllFloatRegisters(), and
 * _LoadThenStoreAllFloatRegisters agree on the format.
 *
 * @return N/A
 */

static inline void _LoadAllFloatRegisters(FP_REG_SET *pFromBuffer)
{
	/*
	 * The 'movdqu' is the "move double quad unaligned" instruction: Move
	 * a double quadword (16 bytes) between memory and an XMM register (or
	 * between a pair of XMM registers). The memory destination/source operand
	 * may be unaligned on a 16-byte boundary without causing an exception.
	 *
	 * The 'fldt' is the "load floating point value" instruction: Push an 80-bit
	 * (double extended-precision) onto the FPU register stack.
	 *
	 * A note about operand size specification in the AT&T assembler syntax:
	 *
	 *   Instructions are generally suffixed with the a letter or a pair of
	 *   letters to specify the operand size:
	 *
	 *    b  = byte (8 bit)
	 *    s  = short (16 bit integer) or single (32-bit floating point)
	 *    w  = word (16 bit)
	 *    l  = long (32 bit integer or 64-bit floating point)
	 *    q  = quad (64 bit)
	 *    t  = ten bytes (80-bit floating point)
	 *    dq = double quad (128 bit)
	 */

	__asm__ volatile (
		"movdqu  0(%0), %%xmm0\n\t;"
		"movdqu 16(%0), %%xmm1\n\t;"
		"movdqu 32(%0), %%xmm2\n\t;"
		"movdqu 48(%0), %%xmm3\n\t;"
		"movdqu 64(%0), %%xmm4\n\t;"
		"movdqu 80(%0), %%xmm5\n\t;"
		"movdqu 96(%0), %%xmm6\n\t;"
		"movdqu 112(%0), %%xmm7\n\t;"

		"fldt   128(%0)\n\t;"
		"fldt   138(%0)\n\t;"
		"fldt   148(%0)\n\t;"
		"fldt   158(%0)\n\t;"
		"fldt   168(%0)\n\t;"
		"fldt   178(%0)\n\t;"
		"fldt   188(%0)\n\t;"
		"fldt   198(%0)\n\t;"

		:: "r" (pFromBuffer)
		);
}


/**
 *
 * @brief Load then dump all float registers to memory
 *
 * This function loads ALL floating point registers from the memory buffer
 * specified by <pFromToBuffer>, and then stores them back to that buffer.
 *
 * This routine is called by a high priority thread prior to calling a primitive
 * that pends and triggers a co-operative context switch to a low priority
 * thread. Because the kernel doesn't save floating point context for
 * co-operative context switches, the x87 FPU register stack must be put back
 * in an empty state before the switch occurs in case the next task to perform
 * floating point operations was also co-operatively switched out and simply
 * inherits the existing x87 FPU state (expecting the stack to be empty).
 *
 * @return N/A
 */

static inline void _LoadThenStoreAllFloatRegisters(FP_REG_SET *pFromToBuffer)
{
	__asm__ volatile (
		"movdqu  0(%0), %%xmm0\n\t;"
		"movdqu 16(%0), %%xmm1\n\t;"
		"movdqu 32(%0), %%xmm2\n\t;"
		"movdqu 48(%0), %%xmm3\n\t;"
		"movdqu 64(%0), %%xmm4\n\t;"
		"movdqu 80(%0), %%xmm5\n\t;"
		"movdqu 96(%0), %%xmm6\n\t;"
		"movdqu 112(%0), %%xmm7\n\t;"

		"fldt   128(%0)\n\t;"
		"fldt   138(%0)\n\t;"
		"fldt   148(%0)\n\t;"
		"fldt   158(%0)\n\t;"
		"fldt   168(%0)\n\t;"
		"fldt   178(%0)\n\t;"
		"fldt   188(%0)\n\t;"
		"fldt   198(%0)\n\t;"

		/* pop the x87 FPU registers back to memory */

		"fstpt  198(%0)\n\t;"
		"fstpt  188(%0)\n\t;"
		"fstpt  178(%0)\n\t;"
		"fstpt  168(%0)\n\t;"
		"fstpt  158(%0)\n\t;"
		"fstpt  148(%0)\n\t;"
		"fstpt  138(%0)\n\t;"
		"fstpt  128(%0)\n\t;"

		:: "r" (pFromToBuffer)
		);
}


/**
 *
 * @brief Dump all floating point registers to memory
 *
 * This function stores ALL floating point registers to the memory buffer
 * specified by <pToBuffer>. It is expected that a previous invocation of
 * _LoadAllFloatRegisters() occurred to load all the floating point registers
 * from a memory buffer.
 *
 * @return N/A
 */

static inline void _StoreAllFloatRegisters(FP_REG_SET *pToBuffer)
{
	__asm__ volatile (
		"movdqu %%xmm0, 0(%0)\n\t;"
		"movdqu %%xmm1, 16(%0)\n\t;"
		"movdqu %%xmm2, 32(%0)\n\t;"
		"movdqu %%xmm3, 48(%0)\n\t;"
		"movdqu %%xmm4, 64(%0)\n\t;"
		"movdqu %%xmm5, 80(%0)\n\t;"
		"movdqu %%xmm6, 96(%0)\n\t;"
		"movdqu %%xmm7, 112(%0)\n\t;"

		"fstpt  198(%0)\n\t;"
		"fstpt  188(%0)\n\t;"
		"fstpt  178(%0)\n\t;"
		"fstpt  168(%0)\n\t;"
		"fstpt  158(%0)\n\t;"
		"fstpt  148(%0)\n\t;"
		"fstpt  138(%0)\n\t;"
		"fstpt  128(%0)\n\t;"

		:: "r" (pToBuffer) : "memory"
		);
}

/**
 *
 * @brief Dump non-volatile FP registers to memory
 *
 * This routine is called by a high priority thread after resuming execution
 * from calling a primitive that will pend and thus result in a co-operative
 * context switch to a low priority thread.
 *
 * Only the non-volatile floating point registers are expected to survive across
 * a function call, regardless of whether the call results in the thread being
 * pended.
 *
 * @return N/A
 */

void _StoreNonVolatileFloatRegisters(FP_NONVOLATILE_REG_SET *pToBuffer)
{
	ARG_UNUSED(pToBuffer);
	/* do nothing; there are no non-volatile floating point registers */
}
#endif /* _FLOAT_REGS_X86_GCC_H */
