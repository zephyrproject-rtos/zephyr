/*
 * Copyright (c) 2015 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public interface for configuring interrupts
 */
#ifndef ZEPHYR_INCLUDE_IRQ_H_
#define ZEPHYR_INCLUDE_IRQ_H_

/* Pull in the arch-specific implementations */
#include <arch/cpu.h>

#ifndef _ASMLANGUAGE
#include <toolchain.h>
#include <zephyr/types.h>

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
 */
#define IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
	ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p)

/**
 * Configure a dynamic interrupt.
 *
 * Use this instead of IRQ_CONNECT() if arguments cannot be known at build time.
 *
 * @param irq IRQ line number
 * @param priority Interrupt priority
 * @param routine Interrupt service routine
 * @param parameter ISR parameter
 * @param flags Arch-specific IRQ configuration flags
 *
 * @return The vector assigned to this interrupt
 */
static inline int
irq_connect_dynamic(unsigned int irq, unsigned int priority,
		    void (*routine)(const void *parameter),
		    const void *parameter, uint32_t flags)
{
	return arch_irq_connect_dynamic(irq, priority, routine, parameter,
					flags);
}

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
 */
#define IRQ_DIRECT_CONNECT(irq_p, priority_p, isr_p, flags_p) \
	ARCH_IRQ_DIRECT_CONNECT(irq_p, priority_p, isr_p, flags_p)

/**
 * @brief Common tasks before executing the body of an ISR
 *
 * This macro must be at the beginning of all direct interrupts and performs
 * minimal architecture-specific tasks before the ISR itself can run. It takes
 * no arguments and has no return value.
 */
#define ISR_DIRECT_HEADER() ARCH_ISR_DIRECT_HEADER()

/**
 * @brief Common tasks before exiting the body of an ISR
 *
 * This macro must be at the end of all direct interrupts and performs
 * minimal architecture-specific tasks like EOI. It has no return value.
 *
 * In a normal interrupt, a check is done at end of interrupt to invoke
 * z_swap() logic if the current thread is preemptible and there is another
 * thread ready to run in the kernel's ready queue cache. This is now optional
 * and controlled by the check_reschedule argument. If unsure, set to nonzero.
 * On systems that do stack switching and nested interrupt tracking in software,
 * z_swap() should only be called if this was a non-nested interrupt.
 *
 * @param check_reschedule If nonzero, additionally invoke scheduling logic
 */
#define ISR_DIRECT_FOOTER(check_reschedule) \
	ARCH_ISR_DIRECT_FOOTER(check_reschedule)

/**
 * @brief Perform power management idle exit logic
 *
 * This macro may optionally be invoked somewhere in between IRQ_DIRECT_HEADER()
 * and IRQ_DIRECT_FOOTER() invocations. It performs tasks necessary to
 * exit power management idle state. It takes no parameters and returns no
 * arguments. It may be omitted, but be careful!
 */
#define ISR_DIRECT_PM() ARCH_ISR_DIRECT_PM()

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
 *		return 0;  <-- Don't bother checking if we have to z_swap()
 *	}
 *	k_sem_give(some_sem);
 *	return 1;
 * }
 *
 * @param name symbol name of the ISR
 */
#define ISR_DIRECT_DECLARE(name) ARCH_ISR_DIRECT_DECLARE(name)

/**
 * @brief Lock interrupts.
 * @def irq_lock()
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
 * This routine can only be invoked from supervisor mode. Some architectures
 * (for example, ARM) will fail silently if invoked from user mode instead
 * of generating an exception.
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
 * @return An architecture-dependent lock-out key representing the
 *         "interrupt disable state" prior to the call.
 */
#ifdef CONFIG_SMP
unsigned int z_smp_global_lock(void);
#define irq_lock() z_smp_global_lock()
#else
#define irq_lock() arch_irq_lock()
#endif

/**
 * @brief Unlock interrupts.
 * @def irq_unlock()
 *
 * This routine reverses the effect of a previous call to irq_lock() using
 * the associated lock-out key. The caller must call the routine once for
 * each time it called irq_lock(), supplying the keys in the reverse order
 * they were acquired, before interrupts are enabled.
 *
 * This routine can only be invoked from supervisor mode. Some architectures
 * (for example, ARM) will fail silently if invoked from user mode instead
 * of generating an exception.
 *
 * @note Can be called by ISRs.
 *
 * @param key Lock-out key generated by irq_lock().
 *
 * @return N/A
 */
#ifdef CONFIG_SMP
void z_smp_global_unlock(unsigned int key);
#define irq_unlock(key) z_smp_global_unlock(key)
#else
#define irq_unlock(key) arch_irq_unlock(key)
#endif

/**
 * @brief Return IRQ level
 * @def irq_get_level()
 *
 * This routine returns the interrupt level number of the provided interrupt.
 *
 * @param irq IRQ number in its zephyr format
 *
 * @return 1 if IRQ level 1, 2 if IRQ level 2, 3 if IRQ level 3
 */
static inline unsigned int irq_get_level(unsigned int irq)
{
#if defined(CONFIG_3RD_LEVEL_INTERRUPTS)
	return ((irq >> 16) & 0xFF) != 0 ? 3 :
		(((irq >> 8) & 0xFF) == 0 ? 1 : 2);
#elif defined(CONFIG_2ND_LEVEL_INTERRUPTS)
	return ((irq >> 8) & 0xFF) == 0 ? 1 : 2;
#else
	ARG_UNUSED(irq);

	return 1;
#endif
}

#ifdef CONFIG_2ND_LEVEL_INTERRUPTS
/**
 * @brief Return the 2nd level interrupt number
 * @def irq_from_level_2()
 *
 * This routine returns the second level irq number of the zephyr irq
 * number passed in
 *
 * @param irq IRQ number in its zephyr format
 *
 * @return 2nd level IRQ number
 */
static inline unsigned int irq_from_level_2(unsigned int irq)
{
#ifdef CONFIG_3RD_LEVEL_INTERRUPTS
	return ((irq >> 8) & 0xFF) - 1;
#else
	return (irq >> 8) - 1;
#endif
}

/**
 * @brief Converts irq from level 1 to level 2 format
 * @def irq_to_level_2()
 *
 * This routine converts the input into the level 2 irq number format
 *
 * @note Values >= 0xFF are invalid
 *
 * @param irq IRQ number in its zephyr format
 *
 * @return 2nd level IRQ number
 */
static inline unsigned int irq_to_level_2(unsigned int irq)
{
	return (irq + 1) << 8;
}

/**
 * @brief Returns the parent IRQ of the level 2 raw IRQ number
 * @def irq_parent_level_2()
 *
 * The parent of a 2nd level interrupt is in the 1st byte
 *
 * @param irq IRQ number in its zephyr format
 *
 * @return 2nd level IRQ parent
 */
static inline unsigned int irq_parent_level_2(unsigned int irq)
{
	return irq & 0xFF;
}
#endif

#ifdef CONFIG_3RD_LEVEL_INTERRUPTS
/**
 * @brief Return the 3rd level interrupt number
 * @def irq_from_level_3()
 *
 * This routine returns the third level irq number of the zephyr irq
 * number passed in
 *
 * @param irq IRQ number in its zephyr format
 *
 * @return 3rd level IRQ number
 */
static inline unsigned int irq_from_level_3(unsigned int irq)
{
	return (irq >> 16) - 1;
}

/**
 * @brief Converts irq from level 1 to level 3 format
 * @def irq_to_level_3()
 *
 * This routine converts the input into the level 3 irq number format
 *
 * @note Values >= 0xFF are invalid
 *
 * @param irq IRQ number in its zephyr format
 *
 * @return 3rd level IRQ number
 */
static inline unsigned int irq_to_level_3(unsigned int irq)
{
	return (irq + 1) << 16;
}

/**
 * @brief Returns the parent IRQ of the level 3 raw IRQ number
 * @def irq_parent_level_3()
 *
 * The parent of a 3rd level interrupt is in the 2nd byte
 *
 * @param irq IRQ number in its zephyr format
 *
 * @return 3rd level IRQ parent
 */
static inline unsigned int irq_parent_level_3(unsigned int irq)
{
	return (irq >> 8) & 0xFF;
}
#endif

/**
 * @brief Enable an IRQ.
 *
 * This routine enables interrupts from source @a irq.
 *
 * @param irq IRQ line.
 *
 * @return N/A
 */
#define irq_enable(irq) arch_irq_enable(irq)

/**
 * @brief Disable an IRQ.
 *
 * This routine disables interrupts from source @a irq.
 *
 * @param irq IRQ line.
 *
 * @return N/A
 */
#define irq_disable(irq) arch_irq_disable(irq)

/**
 * @brief Get IRQ enable state.
 *
 * This routine indicates if interrupts from source @a irq are enabled.
 *
 * @param irq IRQ line.
 *
 * @return interrupt enable state, true or false
 */
#define irq_is_enabled(irq) arch_irq_is_enabled(irq)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_IRQ_H_ */
