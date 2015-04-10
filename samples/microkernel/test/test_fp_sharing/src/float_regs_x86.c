/* float_regs_x86.c - floating point register load/store APIs for IA-32  */

/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
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

/*
DESCRIPTION
This module provides implementations of the _LoadAllFloatRegisters(),
_StoreAllFloatRegisters(), _LoadThenStoreAllFloatRegisters, and
_StoreNonVolatileFloatRegisters() routines required by the 'floatContext' test.
*/

#if defined(CONFIG_ISA_IA32)

#include <toolchain.h>
#include "float_context.h"

#if defined(__DCC__)
__asm volatile void _LoadAllFloatRegisters_inline(FP_REG_SET *pFromBuffer)
{
% reg pFromBuffer
!
	movdqu  0(pFromBuffer), %xmm0
	movdqu 16(pFromBuffer), %xmm1
	movdqu 32(pFromBuffer), %xmm2
	movdqu 48(pFromBuffer), %xmm3
	movdqu 64(pFromBuffer), %xmm4
	movdqu 80(pFromBuffer), %xmm5
	movdqu 96(pFromBuffer), %xmm6
	movdqu 112(pFromBuffer), %xmm7

	fldt   128(pFromBuffer)
	fldt   138(pFromBuffer)
	fldt   148(pFromBuffer)
	fldt   158(pFromBuffer)
	fldt   168(pFromBuffer)
	fldt   178(pFromBuffer)
	fldt   188(pFromBuffer)
	fldt   198(pFromBuffer)
}
#endif

/*******************************************************************************
*
* _LoadAllFloatRegisters - load all floating point registers
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
* RETURNS: N/A
*/

void _LoadAllFloatRegisters (FP_REG_SET *pFromBuffer)
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

#if defined(__GNUC__)
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
#elif defined(__DCC__)
	_LoadAllFloatRegisters_inline (pFromBuffer);
#endif
}

#if defined(__DCC__)
__asm volatile void _LoadThenStoreAllFloatRegisters_inline(
	FP_REG_SET *pFromToBuffer)
{
% reg pFromToBuffer
!
	movdqu  0(pFromToBuffer), %xmm0
	movdqu 16(pFromToBuffer), %xmm1
	movdqu 32(pFromToBuffer), %xmm2
	movdqu 48(pFromToBuffer), %xmm3
	movdqu 64(pFromToBuffer), %xmm4
	movdqu 80(pFromToBuffer), %xmm5
	movdqu 96(pFromToBuffer), %xmm6
	movdqu 112(pFromToBuffer), %xmm7

	fldt   128(pFromToBuffer)
	fldt   138(pFromToBuffer)
	fldt   148(pFromToBuffer)
	fldt   158(pFromToBuffer)
	fldt   168(pFromToBuffer)
	fldt   178(pFromToBuffer)
	fldt   188(pFromToBuffer)
	fldt   198(pFromToBuffer)

	/* pop the x87 FPU registers back to memory */

	fstpt  198(pFromToBuffer)
	fstpt  188(pFromToBuffer)
	fstpt  178(pFromToBuffer)
	fstpt  168(pFromToBuffer)
	fstpt  158(pFromToBuffer)
	fstpt  148(pFromToBuffer)
	fstpt  138(pFromToBuffer)
	fstpt  128(pFromToBuffer)
}
#endif

/*******************************************************************************
*
* _LoadThenStoreAllFloatRegisters - load then dump all float registers to memory
*
* This function loads ALL floating point registers from the memory buffer
* specified by <pFromToBuffer>, and then stores them back to that buffer.
*
* This routine is called by a high priority context prior to calling a primitive
* that pends and triggers a co-operative context switch to a low priority
* context. Because the kernel doesn't save floating point context for
* co-operative context switches, the x87 FPU register stack must be put back
* in an empty state before the switch occurs in case the next task to perform
* floating point operations was also co-operatively switched out and simply
* inherits the existing x87 FPU state (expecting the stack to be empty).
*
* RETURNS: N/A
*/

void _LoadThenStoreAllFloatRegisters (FP_REG_SET *pFromToBuffer)
{
#if defined(__GNUC__)
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
#elif defined(__DCC__)
	_LoadThenStoreAllFloatRegisters_inline (pFromToBuffer);
#endif
}


#if defined(__DCC__)
__asm volatile void _StoreAllFloatRegisters_inline(FP_REG_SET *pToBuffer)
{
% reg pToBuffer
!
	movdqu %xmm0, 0(pToBuffer)
	movdqu %xmm1, 16(pToBuffer)
	movdqu %xmm2, 32(pToBuffer)
	movdqu %xmm3, 48(pToBuffer)
	movdqu %xmm4, 64(pToBuffer)
	movdqu %xmm5, 80(pToBuffer)
	movdqu %xmm6, 96(pToBuffer)
	movdqu %xmm7, 112(pToBuffer)

	fstpt  198(pToBuffer)
	fstpt  188(pToBuffer)
	fstpt  178(pToBuffer)
	fstpt  168(pToBuffer)
	fstpt  158(pToBuffer)
	fstpt  148(pToBuffer)
	fstpt  138(pToBuffer)
	fstpt  128(pToBuffer)
}
#endif

/*******************************************************************************
*
* _StoreAllFloatRegisters - dump all floating point registers to memory
*
* This function stores ALL floating point registers to the memory buffer
* specified by <pToBuffer>. It is expected that a previous invocation of
* _LoadAllFloatRegisters() occured to load all the floating point registers
* from a memory buffer.
*
* RETURNS: N/A
*/

void _StoreAllFloatRegisters (FP_REG_SET *pToBuffer)
{
#if defined(__GNUC__)
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
#elif defined(__DCC__)
	_StoreAllFloatRegisters_inline (pToBuffer);
#endif
}


/*******************************************************************************
*
* _StoreNonVolatileFloatRegisters - dump non-volatile FP registers to memory
*
* This routine is called by a high priority context after resuming execution
* from calling a primitive that will pend and thus result in a co-operative
* context switch to a low priority context.
*
* Only the non-volatile floating point registers are expected to survive across
* a function call, regardless of whether the call results in the context being
* pended.
*
* RETURNS: N/A
*/

void _StoreNonVolatileFloatRegisters (FP_NONVOLATILE_REG_SET *pToBuffer)
{
	ARG_UNUSED (pToBuffer);
	/* do nothing; there are no non-volatile floating point registers */
}

#endif /* CONFIG_ISA_IA32 */
