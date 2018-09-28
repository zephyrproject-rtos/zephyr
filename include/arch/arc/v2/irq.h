/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARCv2 public interrupt handling
 *
 * ARCv2 kernel interrupt handling interface. Included by arc/arch.h.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARC_V2_IRQ_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_V2_IRQ_H_

#include <arch/arc/v2/aux_regs.h>
#include <toolchain/common.h>
#include <irq.h>
#include <misc/util.h>
#include <sw_isr_table.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE
GTEXT(_irq_exit);
GTEXT(_arch_irq_enable)
GTEXT(_arch_irq_disable)
#else

extern void _arch_irq_enable(unsigned int irq);
extern void _arch_irq_disable(unsigned int irq);

extern void _irq_exit(void);
extern void _irq_priority_set(unsigned int irq, unsigned int prio,
			      u32_t flags);
extern void _isr_wrapper(void);
extern void _irq_spurious(void *unused);

/**
 * Configure a static interrupt.
 *
 * All arguments must be computable by the compiler at build time.
 *
 * _ISR_DECLARE will populate the .intList section with the interrupt's
 * parameters, which will then be used by gen_irq_tables.py to create
 * the vector table and the software ISR table. This is all done at
 * build-time.
 *
 * We additionally set the priority in the interrupt controller at
 * runtime.
 *
 * @param irq_p IRQ line number
 * @param priority_p Interrupt priority
 * @param isr_p Interrupt service routine
 * @param isr_param_p ISR parameter
 * @param flags_p IRQ options
 *
 * @return The vector assigned to this interrupt
 */
#define _ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
({ \
	_ISR_DECLARE(irq_p, 0, isr_p, isr_param_p); \
	_irq_priority_set(irq_p, priority_p, flags_p); \
	irq_p; \
})



/**
 *
 * @brief Disable all interrupts on the local CPU
 *
 * This routine disables interrupts.  It can be called from either interrupt or
 * thread level.  This routine returns an architecture-dependent
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
 * thread disables interrupts and subsequently invokes a kernel
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

	__asm__ volatile("clri %0" : "=r"(key):: "memory");
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
 * This routine can be called from either interrupt or thread level.
 *
 * @return N/A
 */

static ALWAYS_INLINE void _arch_irq_unlock(unsigned int key)
{
	__asm__ volatile("seti %0" : : "ir"(key) : "memory");
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARC_V2_IRQ_H_ */
