/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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

/**
 * @file
 * @brief ARCv2 public interrupt handling
 *
 * ARCv2 nanokernel interrupt handling interface. Included by ARC/v2/arch.h.
 */

#ifndef _ARCH_ARC_V2_IRQ__H_
#define _ARCH_ARC_V2_IRQ__H_

#include <arch/arc/v2/aux_regs.h>
#include <toolchain/common.h>
#include <irq.h>
#include <misc/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE
GTEXT(_irq_exit);
GTEXT(_arch_irq_connect)
GTEXT(_arch_irq_enable)
GTEXT(_arch_irq_disable)
#else

extern void _arch_irq_enable(unsigned int irq);
extern void _arch_irq_disable(unsigned int irq);

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

static ALWAYS_INLINE unsigned int _arch_irq_lock(void)
{
	unsigned int key;

	__asm__ volatile("clri %0" : "=r"(key));
	return key;
}

/**
 *
 * @brief Enable all interrupts on the local CPU
 *
 * This routine re-enables interrupts on the local CPU.  The @a key parameter
 * is an architecture-dependent lock-out key that is returned by a previous
 * invocation of irq_lock().
 *
 * This routine can be called from either interrupt, task or fiber level.
 *
 * @return N/A
 */

static ALWAYS_INLINE void _arch_irq_unlock(unsigned int key)
{
	__asm__ volatile("seti %0" : : "ir"(key));
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _ARCH_ARC_V2_IRQ__H_ */
