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
 * by public inline functions and macros are described in
 * include/sys/arch_inlines.h.
 *
 * For all inline functions prototyped here, the implementation is expected
 * to be provided by arch/ARCH/include/kernel_arch_func.h
 *
 * This header is not intended for general use; like kernel_arch_func.h,
 * it is intended to be pulled in by internal kernel headers, specifically
 * kernel/include/kernel_structs.h
 */
#ifndef ZEPHYR_INCLUDE_SYS_ARCH_INTERFACE_H_
#define ZEPHYR_INCLUDE_SYS_ARCH_INTERFACE_H_

#ifndef _ASMLANGUAGE
#include <kernel.h>

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
void z_arch_busy_wait(u32_t usec_to_wait);
#endif

/** @} */

/**
 * @defgroup arch-threads Architecture thread APIs
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
void z_arch_new_thread(struct k_thread *thread, k_thread_stack_t *pStack,
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
 * Some arches may implement thread->switch handle as a pointer to the thread
 * itself, and save context somewhere in thread->arch. In this case, on initial
 * context switch from the dummy thread, thread->switch handle for the outgoing
 * thread is NULL. Instead of dereferencing switched_from all the way to get
 * the thread pointer, subtract ___thread_t_switch_handle_OFFSET to obtain the
 * thread pointer instead.
 *
 * @param switch_to Incoming thread's switch handle
 * @param switched_from Pointer to outgoing thread's switch handle storage
 *        location, which may be updated.
 */
static inline void z_arch_switch(void *switch_to, void **switched_from);
#else
/**
 * Cooperatively context switch
 *
 * Must be called with interrupts locked with the provided key.
 * This is the older-style context switching method, which is incompatible
 * with SMP. New arch ports, either SMP or UP, are encouraged to implement
 * z_arch_switch() instead.
 *
 * @param key Interrupt locking key
 * @return If woken from blocking on some kernel object, the result of that
 *         blocking operation.
 */
int z_arch_swap(unsigned int key);

/**
 * Set the return value for the specified thread.
 *
 * It is assumed that the specified @a thread is pending.
 *
 * @param thread Pointer to thread object
 * @param value value to set as return value
 */
static ALWAYS_INLINE void
z_arch_thread_return_value_set(struct k_thread *thread, unsigned int value);
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
void z_arch_switch_to_main_thread(struct k_thread *main_thread,
				  k_thread_stack_t *main_stack,
				  size_t main_stack_size,
				  k_thread_entry_t _main);
#endif /* CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN */

#if defined(CONFIG_FLOAT) && defined(CONFIG_FP_SHARING)
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
int z_arch_float_disable(struct k_thread *thread);
#endif /* CONFIG_FLOAT && CONFIG_FP_SHARING */

/** @} */

/**
 * @defgroup arch-pm Architecture-specific power management APIs
 * @{
 */
/** Halt the system, optionally propagating a reason code */
FUNC_NORETURN void z_arch_system_halt(unsigned int reason);

/** @} */


/**
 * @defgroup arch-smp Architecture-specific SMP APIs
 * @{
 */
#ifdef CONFIG_SMP
/** Return the CPU struct for the currently executing CPU */
static inline struct _cpu *z_arch_curr_cpu(void);

/**
 * Broadcast an interrupt to all CPUs
 *
 * This will invoke z_sched_ipi() on other CPUs in the system.
 */
void z_arch_sched_ipi(void);
#endif /* CONFIG_SMP */

/** @} */


/**
 * @defgroup arch-irq Architecture-specific IRQ APIs
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
static inline bool z_arch_is_in_isr(void);

/** @} */


/**
 * @defgroup arch-userspace Architecture-specific userspace APIs
 * @{
 */
#ifdef CONFIG_USERSPACE
/**
 * @brief Get the maximum number of partitions for a memory domain
 *
 * @return Max number of partitions, or -1 if there is no limit
 */
int z_arch_mem_domain_max_partitions_get(void);

/**
 * @brief Add a thread to a memory domain (arch-specific)
 *
 * Architecture-specific hook to manage internal data structures or hardware
 * state when the provided thread has been added to a memory domain.
 *
 * The thread's memory domain pointer will be set to the domain to be added
 * to.
 *
 * @param thread Thread which needs to be configured.
 */
void z_arch_mem_domain_thread_add(struct k_thread *thread);

/**
 * @brief Remove a thread from a memory domain (arch-specific)
 *
 * Architecture-specific hook to manage internal data structures or hardware
 * state when the provided thread has been removed from a memory domain.
 *
 * The thread's memory domain pointer will be the domain that the thread
 * is being removed from.
 *
 * @param thread Thread being removed from its memory domain
 */
void z_arch_mem_domain_thread_remove(struct k_thread *thread);

/**
 * @brief Remove a partition from the memory domain (arch-specific)
 *
 * Architecture-specific hook to manage internal data structures or hardware
 * state when a memory domain has had a partition removed.
 *
 * The partition index data, and the number of partitions configured, are not
 * respectively cleared and decremented in the domain until after this function
 * runs.
 *
 * @param domain The memory domain structure
 * @param partition_id The partition index that needs to be deleted
 */
void z_arch_mem_domain_partition_remove(struct k_mem_domain *domain,
					u32_t partition_id);

/**
 * @brief Add a partition to the memory domain
 *
 * Architecture-specific hook to manage internal data structures or hardware
 * state when a memory domain has a partition added.
 *
 * @param domain The memory domain structure
 * @param partition_id The partition that needs to be added
 */
void z_arch_mem_domain_partition_add(struct k_mem_domain *domain,
				     u32_t partition_id);

/**
 * @brief Remove the memory domain
 *
 * Architecture-specific hook to manage internal data structures or hardware
 * state when a memory domain has been destroyed.
 *
 * Thread assignments to the memory domain are only cleared after this function
 * runs.
 *
 * @param domain The memory domain structure which needs to be deleted.
 */
void z_arch_mem_domain_destroy(struct k_mem_domain *domain);

/**
 * @brief Check memory region permissions
 *
 * Given a memory region, return whether the current memory management hardware
 * configuration would allow a user thread to read/write that region. Used by
 * system calls to validate buffers coming in from userspace.
 *
 * Notes:
 * The function is guaranteed to never return validation success, if the entire
 * buffer area is not user accessible.
 *
 * The function is guaranteed to correctly validate the permissions of the
 * supplied buffer, if the user access permissions of the entire buffer are
 * enforced by a single, enabled memory management region.
 *
 * In some architectures the validation will always return failure
 * if the supplied memory buffer spans multiple enabled memory management
 * regions (even if all such regions permit user access).
 *
 * @param addr start address of the buffer
 * @param size the size of the buffer
 * @param write If nonzero, additionally check if the area is writable.
 *	  Otherwise, just check if the memory can be read.
 *
 * @return nonzero if the permissions don't match.
 */
int z_arch_buffer_validate(void *addr, size_t size, int write);

/**
 * Perform a one-way transition from supervisor to kernel mode.
 *
 * Implementations of this function must do the following:
 *
 * - Reset the thread's stack pointer to a suitable initial value. We do not
 *   need any prior context since this is a one-way operation.
 * - Set up any kernel stack region for the CPU to use during privilege
 *   elevation
 * - Put the CPU in whatever its equivalent of user mode is
 * - Transfer execution to z_arch_new_thread() passing along all the supplied
 *   arguments, in user mode.
 *
 * @param user_entry Entry point to start executing as a user thread
 * @param p1 1st parameter to user thread
 * @param p2 2nd parameter to user thread
 * @param p3 3rd parameter to user thread
 */
FUNC_NORETURN void z_arch_user_mode_enter(k_thread_entry_t user_entry,
					  void *p1, void *p2, void *p3);

/**
 * @brief Induce a kernel oops that appears to come from a specific location
 *
 * Normally, k_oops() generates an exception that appears to come from the
 * call site of the k_oops() itself.
 *
 * However, when validating arguments to a system call, if there are problems
 * we want the oops to appear to come from where the system call was invoked
 * and not inside the validation function.
 *
 * @param ssf System call stack frame pointer. This gets passed as an argument
 *            to _k_syscall_handler_t functions and its contents are completely
 *            architecture specific.
 */
FUNC_NORETURN void z_arch_syscall_oops(void *ssf);

/**
 * @brief Safely take the length of a potentially bad string
 *
 * This must not fault, instead the err parameter must have -1 written to it.
 * This function otherwise should work exactly like libc strnlen(). On success
 * *err should be set to 0.
 *
 * @param s String to measure
 * @param maxsize Max length of the string
 * @param err Error value to write
 * @return Length of the string, not counting NULL byte, up to maxsize
 */
size_t z_arch_user_string_nlen(const char *s, size_t maxsize, int *err);
#endif /* CONFIG_USERSPACE */

/** @} */


/**
 * @defgroup arch-benchmarking Architecture-specific benchmarking globals
 */

#ifdef CONFIG_EXECUTION_BENCHMARKING
extern u64_t z_arch_timing_swap_start;
extern u64_t z_arch_timing_swap_end;
extern u64_t z_arch_timing_irq_start;
extern u64_t z_arch_timing_irq_end;
extern u64_t z_arch_timing_tick_start;
extern u64_t z_arch_timing_tick_end;
extern u64_t z_arch_timing_user_mode_end;
extern u32_t z_arch_timing_value_swap_end;
extern u64_t z_arch_timing_value_swap_common;
extern u64_t z_arch_timing_value_swap_temp;
#endif /* CONFIG_EXECUTION_BENCHMARKING */

/** @} */


/**
 * @defgroup arch-misc Miscellaneous architecture APIs
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
static inline void z_arch_kernel_init(void);

/** Do nothing and return. Yawn. */
static inline void z_arch_nop(void);

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_SYS_ARCH_INTERFACE_H_ */
