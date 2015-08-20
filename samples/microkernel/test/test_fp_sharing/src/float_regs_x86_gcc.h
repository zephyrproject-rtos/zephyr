/* Intel x86 GCC specific floating point register macros */

/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
