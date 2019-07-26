/*
 * Copyright (c) 2010-2012, 2014-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Architecture-independent private kernel APIs
 *
 * This file contains private kernel APIs that are not architecture-specific.
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_KERNEL_INTERNAL_H_
#define ZEPHYR_KERNEL_INCLUDE_KERNEL_INTERNAL_H_

#include <kernel.h>
#include <stdbool.h>

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

/* Early boot functions */

void z_bss_zero(void);
#ifdef CONFIG_XIP
void z_data_copy(void);
#else
static inline void z_data_copy(void)
{
	/* Do nothing */
}
#endif
FUNC_NORETURN void z_cstart(void);

extern FUNC_NORETURN void z_thread_entry(k_thread_entry_t entry,
			  void *p1, void *p2, void *p3);

/* Implemented by architectures. Only called from z_setup_new_thread. */
extern void z_new_thread(struct k_thread *thread, k_thread_stack_t *pStack,
			size_t stackSize, k_thread_entry_t entry,
			void *p1, void *p2, void *p3,
			int prio, unsigned int options);

extern void z_setup_new_thread(struct k_thread *new_thread,
			      k_thread_stack_t *stack, size_t stack_size,
			      k_thread_entry_t entry,
			      void *p1, void *p2, void *p3,
			      int prio, u32_t options, const char *name);

#if defined(CONFIG_FLOAT) && defined(CONFIG_FP_SHARING)
/**
 * @brief Disable floating point context preservation
 *
 * The function is used to disable the preservation of floating
 * point context information for a particular thread.
 *
 * @note
 * For ARM architecture, disabling floating point preservation
 * - may only be requested for the current thread
 * - cannot be requested in ISRs.
 *
 * @retval 0       On success.
 * @retval -EINVAL If the floating point disabling could not be performed.
 */
extern int z_arch_float_disable(struct k_thread *thread);
#endif /* CONFIG_FLOAT && CONFIG_FP_SHARING */

#ifdef CONFIG_USERSPACE
/**
 * @brief Get the maximum number of partitions for a memory domain
 *
 * @return Max number of partitions, or -1 if there is no limit
 */
extern int z_arch_mem_domain_max_partitions_get(void);

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
extern void z_arch_mem_domain_thread_add(struct k_thread *thread);

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
extern void z_arch_mem_domain_thread_remove(struct k_thread *thread);

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
extern void z_arch_mem_domain_partition_remove(struct k_mem_domain *domain,
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
extern void z_arch_mem_domain_partition_add(struct k_mem_domain *domain,
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
extern void z_arch_mem_domain_destroy(struct k_mem_domain *domain);

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
extern int z_arch_buffer_validate(void *addr, size_t size, int write);

/**
 * Perform a one-way transition from supervisor to kernel mode.
 *
 * Implementations of this function must do the following:
 * - Reset the thread's stack pointer to a suitable initial value. We do not
 *   need any prior context since this is a one-way operation.
 * - Set up any kernel stack region for the CPU to use during privilege
 *   elevation
 * - Put the CPU in whatever its equivalent of user mode is
 * - Transfer execution to z_new_thread() passing along all the supplied
 *   arguments, in user mode.
 *
 * @param Entry point to start executing as a user thread
 * @param p1 1st parameter to user thread
 * @param p2 2nd parameter to user thread
 * @param p3 3rd parameter to user thread
 */
extern FUNC_NORETURN
void z_arch_user_mode_enter(k_thread_entry_t user_entry, void *p1, void *p2,
			   void *p3);


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
extern FUNC_NORETURN void z_arch_syscall_oops(void *ssf);

/**
 * @brief Safely take the length of a potentially bad string
 *
 * This must not fault, instead the err parameter must have -1 written to it.
 * This function otherwise should work exactly like libc strnlen(). On success
 * *err should be set to 0.
 *
 * @param s String to measure
 * @param maxlen Max length of the string
 * @param err Error value to write
 * @return Length of the string, not counting NULL byte, up to maxsize
 */
extern size_t z_arch_user_string_nlen(const char *s, size_t maxsize, int *err);

/**
 * @brief Zero out BSS sections for application shared memory
 *
 * This isn't handled by any platform bss zeroing, and is called from
 * z_cstart() if userspace is enabled.
 */
extern void z_app_shmem_bss_zero(void);
#endif /* CONFIG_USERSPACE */

/**
 * @brief Allocate some memory from the current thread's resource pool
 *
 * Threads may be assigned a resource pool, which will be used to allocate
 * memory on behalf of certain kernel and driver APIs. Memory reserved
 * in this way should be freed with k_free().
 *
 * @param size Memory allocation size
 * @return A pointer to the allocated memory, or NULL if there is insufficient
 * RAM in the pool or the thread has no resource pool assigned
 */
void *z_thread_malloc(size_t size);

/* set and clear essential thread flag */

extern void z_thread_essential_set(void);
extern void z_thread_essential_clear(void);

/* clean up when a thread is aborted */

#if defined(CONFIG_THREAD_MONITOR)
extern void z_thread_monitor_exit(struct k_thread *thread);
#else
#define z_thread_monitor_exit(thread) \
	do {/* nothing */    \
	} while (false)
#endif /* CONFIG_THREAD_MONITOR */

extern void z_smp_init(void);

extern void smp_timer_init(void);

extern u32_t z_early_boot_rand32_get(void);

#if CONFIG_STACK_POINTER_RANDOM
extern int z_stack_adjust_initialized;
#endif

#if defined(CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT)
extern void z_arch_busy_wait(u32_t usec_to_wait);
#endif

/**
 * TODO: document
 */
extern FUNC_NORETURN void z_arch_system_halt(unsigned int reason);

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_KERNEL_INCLUDE_KERNEL_INTERNAL_H_ */
