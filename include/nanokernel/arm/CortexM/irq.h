/* CortexM/irq.h - Cortex-M public interrupt handling */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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
ARM-specific nanokernel interrupt handling interface. Included by ARM/arch.h.
*/

#ifndef _ARCH_ARM_CORTEXM_IRQ_H_
#define _ARCH_ARM_CORTEXM_IRQ_H_

#include <nanokernel/arm/CortexM/nvic.h>

#ifdef _ASMLANGUAGE
GTEXT(_IntExit);
GTEXT(irq_lock)
GTEXT(irq_unlock)
GTEXT(irq_handler_set)
GTEXT(irq_connect)
GTEXT(irq_disconnect)
GTEXT(irq_enable)
GTEXT(irq_disable)
GTEXT(irq_priority_set)
#else
extern int irq_lock(void);
extern void irq_unlock(int key);

extern void irq_handler_set(unsigned int irq,
				 void (*old)(void *arg),
				 void (*new)(void *arg),
				 void *arg);
extern int irq_connect(unsigned int irq,
			     unsigned int prio,
			     void (*isr)(void *arg),
			     void *arg);
extern void irq_disconnect(unsigned int irq);

extern void irq_enable(unsigned int irq);
extern void irq_disable(unsigned int irq);

extern void irq_priority_set(unsigned int irq, unsigned int prio);

extern void _IntExit(void);

/*******************************************************************************
*
* irq_lock_inline - disable all interrupts on the CPU (inline)
*
* This routine disables interrupts.  It can be called from either interrupt,
* task or fiber level.  This routine returns an architecture-dependent
* lock-out key representing the "interrupt disable state" prior to the call;
* this key can be passed to irq_unlock_inline() to re-enable interrupts.
*
* The lock-out key should only be used as the argument to the
* irq_unlock_inline() API.  It should never be used to manually re-enable
* interrupts or to inspect or manipulate the contents of the source register.
*
* WARNINGS
* Invoking a VxMicro routine with interrupts locked may result in
* interrupts being re-enabled for an unspecified period of time.  If the
* called routine blocks, interrupts will be re-enabled while another
* context executes, or while the system is idle.
*
* The "interrupt disable state" is an attribute of a context.  Thus, if a
* fiber or task disables interrupts and subsequently invokes a VxMicro
* system routine that causes the calling context to block, the interrupt
* disable state will be restored when the context is later rescheduled
* for execution.
*
* RETURNS: An architecture-dependent lock-out key representing the
* "interrupt disable state" prior to the call.
*
* \NOMANUAL
*/

#if defined(__GNUC__)
static ALWAYS_INLINE unsigned int irq_lock_inline(void)
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
#elif defined(__DCC__)
__asm volatile unsigned int __irq_lock_inline(unsigned char pri)
{
	% con pri !"r0", "r1" movs r1, #pri mrs r0, BASEPRI msr BASEPRI, r1
}

static ALWAYS_INLINE unsigned int irq_lock_inline(void)
{
	return __irq_lock_inline(_EXC_IRQ_DEFAULT_PRIO);
}
#endif

/*******************************************************************************
*
* irq_unlock_inline - enable all interrupts on the CPU (inline)
*
* This routine re-enables interrupts on the CPU.  The <key> parameter
* is an architecture-dependent lock-out key that is returned by a previous
* invocation of irq_lock_inline().
*
* This routine can be called from either interrupt, task or fiber level.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

#if defined(__GNUC__)
static ALWAYS_INLINE void irq_unlock_inline(unsigned int key)
{
	__asm__ volatile("msr BASEPRI, %0;\n\t" :  : "r"(key));
}
#elif defined(__DCC__)
__asm volatile void irq_unlock_inline(unsigned int key)
{
	% reg key !"r0" msr BASEPRI, key
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* _ARCH_ARM_CORTEXM_IRQ_H_ */
