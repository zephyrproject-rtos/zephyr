/* Intel ARM GCC specific public inline assembler functions and macros */

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

/* Either public functions or macros or invoked by public functions */

#ifndef _ASM_INLINE_GCC_PUBLIC_GCC_H
#define _ASM_INLINE_GCC_PUBLIC_GCC_H

/*
 * The file must not be included directly
 * Include nanokernel/cpu.h instead
 */

#ifdef _ASMLANGUAGE

#define _SCS_BASE_ADDR _PPB_INT_SCS
#define _SCS_ICSR (_SCS_BASE_ADDR + 0xd04)
#define _SCS_ICSR_PENDSV (1 << 28)
#define _SCS_ICSR_UNPENDSV (1 << 27)
#define _SCS_ICSR_RETTOBASE (1 << 11)

#else /* !_ASMLANGUAGE */
#include <stdint.h>
#include <arch/arm/CortexM/nvic.h>

/**
 *
 * @brief Find first set bit (searching from most significant bit)
 *
 * This routine finds the first bit set in the argument passed it and returns
 * the index of that bit.  Bits are numbered starting at 1 from the least
 * significant bit.  A return value of zero indicates that the value passed
 * is zero.
 *
 * @return most significant bit set
 */

static ALWAYS_INLINE unsigned int find_last_set_inline(unsigned int op)
{
	unsigned int bit;

	__asm__ volatile(
		"cmp %1, #0;\n\t"
		"itt ne;\n\t"
		"   clzne %1, %1;\n\t"
		"   rsbne %0, %1, #32;\n\t"
		: "=r"(bit)
		: "r"(op));

	return bit;
}


/**
 *
 * @brief Find first set bit (from the least significant bit)
 *
 * This routine finds the first bit set in the argument passed it and
 * returns the index of that bit.  Bits are numbered starting
 * at 1 from the least significant bit.  A return value of zero indicates that
 * the value passed is zero.
 *
 * @return least significant bit set
 */

static ALWAYS_INLINE unsigned int find_first_set_inline(unsigned int op)
{
	unsigned int bit;

	__asm__ volatile(
		"rsb %0, %1, #0;\n\t"
		"ands %0, %0, %1;\n\t" /* r0 = x & (-x): only LSB set */
		"itt ne;\n\t"
		"   clzne %0, %0;\n\t" /* count leading zeroes */
		"   rsbne %0, %0, #32;\n\t"
		: "=&r"(bit)
		: "r"(op));

	return bit;
}


/**
 *
 * @brief Disable all interrupts on the CPU
 *
 * This routine disables interrupts.  It can be called from either interrupt,
 * task or fiber level.  This routine returns an architecture-dependent
 * lock-out key representing the "interrupt disable state" prior to the call;
 * this key can be passed to irq_unlock() to re-enable interrupts.
 *
 * The lock-out key should only be used as the argument to the irq_unlock()
 * API.  It should never be used to manually re-enable interrupts or to inspect
 * or manipulate the contents of the source register.
 *
 * This function can be called recursively: it will return a key to return the
 * state of interrupt locking to the previous level.
 *
 * WARNINGS
 * Invoking a kernel routine with interrupts locked may result in
 * interrupts being re-enabled for an unspecified period of time.  If the
 * called routine blocks, interrupts will be re-enabled while another
 * context executes, or while the system is idle.
 *
 * The "interrupt disable state" is an attribute of a context.  Thus, if a
 * fiber or task disables interrupts and subsequently invokes a kernel
 * routine that causes the calling context to block, the interrupt
 * disable state will be restored when the context is later rescheduled
 * for execution.
 *
 * @return An architecture-dependent lock-out key representing the
 * "interrupt disable state" prior to the call.
 *
 * @internal
 *
 * On Cortex-M3/M4, this function prevents exceptions of priority lower than
 * the two highest priorities from interrupting the CPU.
 */

static ALWAYS_INLINE unsigned int irq_lock(void)
{
	unsigned int key;

	__asm__ volatile(
		"movs.n %%r1, %1;\n\t"
		"mrs %0, BASEPRI;\n\t"
		"msr BASEPRI, %%r1;\n\t"
		: "=r"(key)
		: "i"(_EXC_IRQ_DEFAULT_PRIO)
		: "r1");

	return key;
}


/**
 *
 * @brief Enable all interrupts on the CPU (inline)
 *
 * This routine re-enables interrupts on the CPU.  The <key> parameter is an
 * architecture-dependent lock-out key that is returned by a previous
 * invocation of irq_lock().
 *
 * This routine can be called from either interrupt, task or fiber level.
 *
 * @return N/A
 */

static ALWAYS_INLINE void irq_unlock(unsigned int key)
{
	__asm__ volatile("msr BASEPRI, %0;\n\t" :  : "r"(key));
}


#endif /* _ASMLANGUAGE */
#endif /* _ASM_INLINE_GCC_PUBLIC_GCC_H */
