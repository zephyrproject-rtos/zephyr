/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Internal kernel APIs implemented at the architecture layer.
 *
 * Not all architecture-specific defines are here, APIs that are used
 * by public functions and macros are defined in include/sys/arch_interface.h.
 *
 * For all inline functions prototyped here, the implementation is expected
 * to be provided by arch/ARCH/include/kernel_arch_func.h
 */
#ifndef ZEPHYR_KERNEL_INCLUDE_KERNEL_ARCH_INTERFACE_H_
#define ZEPHYR_KERNEL_INCLUDE_KERNEL_ARCH_INTERFACE_H_

#include <kernel.h>
#include <sys/arch_interface.h>

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup arch-timing Architecture timing APIs
 * @{
 */
#ifdef CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT
/**
 * Architecture-specific implementation of busy-waiting
 *
 * @param usec_to_wait Wait period, in microseconds
 */
void arch_busy_wait(u32_t usec_to_wait);
#endif

/** @} */

/**
 * @defgroup arch-threads Architecture thread APIs
 * @ingroup arch-interface
 * @{
 */

/** Handle arch-specific logic for setting up new threads
 *
 * The stack and arch-specific thread state variables must be set up
 * such that a later attempt to switch to this thread will succeed
 * and we will enter z_thread_entry with the requested thread and
 * arguments as its parameters.
 *
 * At some point in this function's implementation, z_setup_new_thread() must
 * be called with the true bounds of the available stack buffer within the
 * thread's stack object.
 *
 * @param thread Pointer to uninitialized struct k_thread
 * @param pStack Pointer to the stack space.
 * @param stackSize Stack size in bytes.
 * @param entry Thread entry function.
 * @param p1 1st entry point parameter.
 * @param p2 2nd entry point parameter.
 * @param p3 3rd entry point parameter.
 * @param prio Thread priority.
 * @param options Thread options.
 */
void arch_new_thread(struct k_thread *thread, k_thread_stack_t *pStack,
		       size_t stackSize, k_thread_entry_t entry,
		       void *p1, void *p2, void *p3,
		       int prio, unsigned int options);

#ifdef CONFIG_USE_SWITCH
/**
 * Cooperatively context switch
 *
 * Architectures have considerable leeway on what the specific semantics of
 * the switch handles are, but optimal implementations should do the following
 * if possible:
 *
 * 1) Push all thread state relevant to the context switch to the current stack
 * 2) Update the switched_from parameter to contain the current stack pointer,
 *    after all context has been saved. switched_from is used as an output-
 *    only parameter and its current value is ignored (and can be NULL, see
 *    below).
 * 3) Set the stack pointer to the value provided in switch_to
 * 4) Pop off all thread state from the stack we switched to and return.
 *
 * Some arches may implement thread->switch handle as a pointer to the
 * thread itself, and save context somewhere in thread->arch. In this
 * case, on initial context switch from the dummy thread,
 * thread->switch handle for the outgoing thread is NULL. Instead of
 * dereferencing switched_from all the way to get the thread pointer,
 * subtract ___thread_t_switch_handle_OFFSET to obtain the thread
 * pointer instead.  That is, such a scheme would have behavior like
 * (in C pseudocode):
 *
 * void arch_switch(void *switch_to, void **switched_from)
 * {
 *     struct k_thread *new = switch_to;
 *     struct k_thread *old = CONTAINER_OF(switched_from, struct k_thread,
 *                                         switch_handle);
 *
 *     // save old context...
 *     *switched_from = old;
 *     // restore new context...
 * }
 *
 * Note that, regardless of the underlying handle representation, the
 * incoming switched_from pointer MUST be written through with a
 * non-NULL value after all relevant thread state has been saved.  The
 * kernel uses this as a synchronization signal to be able to wait for
 * switch completion from another CPU.
 *
 * @param switch_to Incoming thread's switch handle
 * @param switched_from Pointer to outgoing thread's switch handle storage
 *        location, which may be updated.
 */
static inline void arch_switch(void *switch_to, void **switched_from);
#else
/**
 * Cooperatively context switch
 *
 * Must be called with interrupts locked with the provided key.
 * This is the older-style context switching method, which is incompatible
 * with SMP. New arch ports, either SMP or UP, are encouraged to implement
 * arch_switch() instead.
 *
 * @param key Interrupt locking key
 * @return If woken from blocking on some kernel object, the result of that
 *         blocking operation.
 */
int arch_swap(unsigned int key);

/**
 * Set the return value for the specified thread.
 *
 * It is assumed that the specified @a thread is pending.
 *
 * @param thread Pointer to thread object
 * @param value value to set as return value
 */
static ALWAYS_INLINE void
arch_thread_return_value_set(struct k_thread *thread, unsigned int value);
#endif /* CONFIG_USE_SWITCH i*/

#ifdef CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN
/**
 * Custom logic for entering main thread context at early boot
 *
 * Used by architectures where the typical trick of setting up a dummy thread
 * in early boot context to "switch out" of isn't workable.
 *
 * @param main_thread main thread object
 * @param main_stack main thread's stack object
 * @param main_stack_size Size of the stack object's buffer
 * @param _main Entry point for application main function.
 */
void arch_switch_to_main_thread(struct k_thread *main_thread,
				  k_thread_stack_t *main_stack,
				  size_t main_stack_size,
				  k_thread_entry_t _main);
#endif /* CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN */

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
/**
 * @brief Disable floating point context preservation
 *
 * The function is used to disable the preservation of floating
 * point context information for a particular thread.
 *
 * @note For ARM architecture, disabling floating point preservation may only
 * be requested for the current thread and cannot be requested in ISRs.
 *
 * @retval 0       On success.
 * @retval -EINVAL If the floating point disabling could not be performed.
 */
int arch_float_disable(struct k_thread *thread);
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

/** @} */

/**
 * @defgroup arch-pm Architecture-specific power management APIs
 * @ingroup arch-interface
 * @{
 */
/** Halt the system, optionally propagating a reason code */
FUNC_NORETURN void arch_system_halt(unsigned int reason);

/** @} */


/**
 * @defgroup arch-irq Architecture-specific IRQ APIs
 * @ingroup arch-interface
 * @{
 */

/**
 * Test if the current context is in interrupt context
 *
 * XXX: This is inconsistently handled among arches wrt exception context
 * See: #17656
 *
 * @return true if we are in interrupt context
 */
static inline bool arch_is_in_isr(void);

/** @} */

/**
 * @defgroup arch-misc Miscellaneous architecture APIs
 * @ingroup arch-interface
 * @{
 */

/**
 * Architecture-specific kernel initialization hook
 *
 * This function is invoked near the top of _Cstart, for additional
 * architecture-specific setup before the rest of the kernel is brought up.
 *
 * TODO: Deprecate, most arches are using a prep_c() function to do the same
 * thing in a simpler way
 */
static inline void arch_kernel_init(void);

/** Do nothing and return. Yawn. */
static inline void arch_nop(void);

/** @} */

/* Include arch-specific inline function implementation */
#include <kernel_arch_func.h>

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_KERNEL_INCLUDE_KERNEL_ARCH_INTERFACE_H_ */
