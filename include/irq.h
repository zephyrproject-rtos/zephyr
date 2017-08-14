/*
 * Copyright (c) 2015 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public interface for configuring interrupts
 */
#ifndef _IRQ_H_
#define _IRQ_H_

/* Pull in the arch-specific implementations */
#include <arch/cpu.h>

#ifndef _ASMLANGUAGE
#include <toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup isr_apis Interrupt Service Routine APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Initialize an interrupt handler.
 *
 * This routine initializes an interrupt handler for an IRQ. The IRQ must be
 * subsequently enabled before the interrupt handler begins servicing
 * interrupts.
 *
 * @warning
 * Although this routine is invoked at run-time, all of its arguments must be
 * computable by the compiler at build time.
 *
 * @param irq_p IRQ line number.
 * @param priority_p Interrupt priority.
 * @param isr_p Address of interrupt service routine.
 * @param isr_param_p Parameter passed to interrupt service routine.
 * @param flags_p Architecture-specific IRQ configuration flags..
 *
 * @return Interrupt vector assigned to this interrupt.
 */
#define IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
	_ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p)

/**
 * @brief Initialize a 'direct' interrupt handler.
 *
 * This routine initializes an interrupt handler for an IRQ. The IRQ must be
 * subsequently enabled via irq_enable() before the interrupt handler begins
 * servicing interrupts.
 *
 * These ISRs are designed for performance-critical interrupt handling and do
 * not go through common interrupt handling code. They must be implemented in
 * such a way that it is safe to put them directly in the vector table.  For
 * ISRs written in C, The ISR_DIRECT_DECLARE() macro will do this
 * automatically. For ISRs written in assembly it is entirely up to the
 * developer to ensure that the right steps are taken.
 *
 * This type of interrupt currently has a few limitations compared to normal
 * Zephyr interrupts:
 * - No parameters are passed to the ISR.
 * - No stack switch is done, the ISR will run on the interrupted context's
 *   stack, unless the architecture automatically does the stack switch in HW.
 * - Interrupt locking state is unchanged from how the HW sets it when the ISR
 *   runs. On arches that enter ISRs with interrupts locked, they will remain
 *   locked.
 * - Scheduling decisions are now optional, controlled by the return value of
 *   ISRs implemented with the ISR_DIRECT_DECLARE() macro
 * - The call into the OS to exit power management idle state is now optional.
 *   Normal interrupts always do this before the ISR is run, but when it runs
 *   is now controlled by the placement of a ISR_DIRECT_PM() macro, or omitted
 *   entirely.
 *
 * @warning
 * Although this routine is invoked at run-time, all of its arguments must be
 * computable by the compiler at build time.
 *
 * @param irq_p IRQ line number.
 * @param priority_p Interrupt priority.
 * @param isr_p Address of interrupt service routine.
 * @param flags_p Architecture-specific IRQ configuration flags.
 *
 * @return Interrupt vector assigned to this interrupt.
 */
#define IRQ_DIRECT_CONNECT(irq_p, priority_p, isr_p, flags_p) \
	_ARCH_IRQ_DIRECT_CONNECT(irq_p, priority_p, isr_p, flags_p)

/**
 * @brief Common tasks before executing the body of an ISR
 *
 * This macro must be at the beginning of all direct interrupts and performs
 * minimal architecture-specific tasks before the ISR itself can run. It takes
 * no arguments and has no return value.
 */
#define ISR_DIRECT_HEADER() _ARCH_ISR_DIRECT_HEADER()

/**
 * @brief Common tasks before exiting the body of an ISR
 *
 * This macro must be at the end of all direct interrupts and performs
 * minimal architecture-specific tasks like EOI. It has no return value.
 *
 * In a normal interrupt, a check is done at end of interrupt to invoke
 * _Swap() logic if the current thread is preemptible and there is another
 * thread ready to run in the kernel's ready queue cache. This is now optional
 * and controlled by the check_reschedule argument. If unsure, set to nonzero.
 * On systems that do stack switching and nested interrupt tracking in software,
 * _Swap() should only be called if this was a non-nested interrupt.
 *
 * @param check_reschedule If nonzero, additionally invoke scheduling logic
 */
#define ISR_DIRECT_FOOTER(check_reschedule) \
	_ARCH_ISR_DIRECT_FOOTER(check_reschedule)

/**
 * @brief Perform power management idle exit logic
 *
 * This macro may optionally be invoked somewhere in between IRQ_DIRECT_HEADER()
 * and IRQ_DIRECT_FOOTER() invocations. It performs tasks necessary to
 * exit power management idle state. It takes no parameters and returns no
 * arguments. It may be omitted, but be careful!
 */
#define ISR_DIRECT_PM() _ARCH_ISR_DIRECT_PM()

/**
 * @brief Helper macro to declare a direct interrupt service routine.
 *
 * This will declare the function in a proper way and automatically include
 * the ISR_DIRECT_FOOTER() and ISR_DIRECT_HEADER() macros. The function should
 * return nonzero status if a scheduling decision should potentially be made.
 * See ISR_DIRECT_FOOTER() for more details on the scheduling decision.
 *
 * For architectures that support 'regular' and 'fast' interrupt types, where
 * these interrupt types require different assembly language handling of
 * registers by the ISR, this will always generate code for the 'fast'
 * interrupt type.
 *
 * Example usage:
 *
 * ISR_DIRECT_DECLARE(my_isr)
 * {
 *	bool done = do_stuff();
 *	ISR_DIRECT_PM(); <-- done after do_stuff() due to latency concerns
 *	if (!done) {
 *		return 0;  <-- Don't bother checking if we have to _Swap()
 *	}
 *	k_sem_give(some_sem);
 *	return 1;
 * }
 *
 * @param name symbol name of the ISR
 */
#define ISR_DIRECT_DECLARE(name) _ARCH_ISR_DIRECT_DECLARE(name)

/**
 * @brief Lock interrupts.
 *
 * This routine disables all interrupts on the CPU. It returns an unsigned
 * integer "lock-out key", which is an architecture-dependent indicator of
 * whether interrupts were locked prior to the call. The lock-out key must be
 * passed to irq_unlock() to re-enable interrupts.
 *
 * This routine can be called recursively, as long as the caller keeps track
 * of each lock-out key that is generated. Interrupts are re-enabled by
 * passing each of the keys to irq_unlock() in the reverse order they were
 * acquired. (That is, each call to irq_lock() must be balanced by
 * a corresponding call to irq_unlock().)
 *
 * @note
 * This routine can be called by ISRs or by threads. If it is called by a
 * thread, the interrupt lock is thread-specific; this means that interrupts
 * remain disabled only while the thread is running. If the thread performs an
 * operation that allows another thread to run (for example, giving a semaphore
 * or sleeping for N milliseconds), the interrupt lock no longer applies and
 * interrupts may be re-enabled while other processing occurs. When the thread
 * once again becomes the current thread, the kernel re-establishes its
 * interrupt lock; this ensures the thread won't be interrupted until it has
 * explicitly released the interrupt lock it established.
 *
 * @warning
 * The lock-out key should never be used to manually re-enable interrupts
 * or to inspect or manipulate the contents of the CPU's interrupt bits.
 *
 * @return Lock-out key.
 */
#define irq_lock() _arch_irq_lock()

/**
 * @brief Unlock interrupts.
 *
 * This routine reverses the effect of a previous call to irq_lock() using
 * the associated lock-out key. The caller must call the routine once for
 * each time it called irq_lock(), supplying the keys in the reverse order
 * they were acquired, before interrupts are enabled.
 *
 * @note Can be called by ISRs.
 *
 * @param key Lock-out key generated by irq_lock().
 *
 * @return N/A
 */
#define irq_unlock(key) _arch_irq_unlock(key)

/**
 * @brief Enable an IRQ.
 *
 * This routine enables interrupts from source @a irq.
 *
 * @param irq IRQ line.
 *
 * @return N/A
 */
#define irq_enable(irq) _arch_irq_enable(irq)

/**
 * @brief Disable an IRQ.
 *
 * This routine disables interrupts from source @a irq.
 *
 * @param irq IRQ line.
 *
 * @return N/A
 */
#define irq_disable(irq) _arch_irq_disable(irq)

/**
 * @brief Get IRQ enable state.
 *
 * This routine indicates if interrupts from source @a irq are enabled.
 *
 * @param irq IRQ line.
 *
 * @return interrupt enable state, true or false
 */
#define irq_is_enabled(irq) _arch_irq_is_enabled(irq)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ASMLANGUAGE */
#endif /* _IRQ_H_ */
