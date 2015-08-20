/* arc/v2/irq.h - ARCv2 public interrupt handling */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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
 * DESCRIPTION
 * ARCv2 nanokernel interrupt handling interface. Included by ARC/v2/arch.h.
 */

#ifndef _ARCH_ARC_V2_IRQ__H_
#define _ARCH_ARC_V2_IRQ__H_

#include <arch/arc/v2/aux_regs.h>
#include <toolchain/common.h>

#ifdef _ASMLANGUAGE
GTEXT(_irq_exit);
GTEXT(irq_connect)
GTEXT(irq_enable)
GTEXT(irq_disable)
#else
extern int irq_connect(unsigned int irq,
			     unsigned int prio,
			     void (*isr)(void *arg),
			     void *arg);

extern void irq_enable(unsigned int irq);
extern void irq_disable(unsigned int irq);

extern void _irq_exit(void);

/**
 *
 * @brief Disable all interrupts on the local CPU
 *
 * This routine disables interrupts.  It can be called from either interrupt,
 * task or fiber level.  This routine returns an architecture-dependent
 * lock-out key representing the "interrupt disable state" prior to the call;
 * this key can be passed to irq_unlock() to re-enable interrupts.
 *
 * The lock-out key should only be used as the argument to the
 * irq_unlock() API.  It should never be used to manually re-enable
 * interrupts or to inspect or manipulate the contents of the source register.
 *
 * This function can be called recursively: it will return a key to return the
 * state of interrupt locking to the previous level.
 *
 * WARNINGS
 * Invoking a kernel routine with interrupts locked may result in
 * interrupts being re-enabled for an unspecified period of time.  If the
 * called routine blocks, interrupts will be re-enabled while another
 * thread executes, or while the system is idle.
 *
 * The "interrupt disable state" is an attribute of a thread.  Thus, if a
 * fiber or task disables interrupts and subsequently invokes a kernel
 * routine that causes the calling thread to block, the interrupt
 * disable state will be restored when the thread is later rescheduled
 * for execution.
 *
 * @return An architecture-dependent lock-out key representing the
 * "interrupt disable state" prior to the call.
 */

static ALWAYS_INLINE unsigned int irq_lock(void)
{
	unsigned int key;

	__asm__ volatile("clri %0" : "=r"(key));
	return key;
}

/**
 *
 * @brief Enable all interrupts on the local CPU
 *
 * This routine re-enables interrupts on the local CPU.  The <key> parameter
 * is an architecture-dependent lock-out key that is returned by a previous
 * invocation of irq_lock().
 *
 * This routine can be called from either interrupt, task or fiber level.
 *
 * @return N/A
 */

static ALWAYS_INLINE void irq_unlock(unsigned int key)
{
	__asm__ volatile("seti %0" : : "ir"(key));
}

#endif /* _ASMLANGUAGE */
#endif /* _ARCH_ARC_V2_IRQ__H_ */
