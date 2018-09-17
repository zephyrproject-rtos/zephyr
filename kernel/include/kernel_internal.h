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

void _bss_zero(void);
#ifdef CONFIG_XIP
void _data_copy(void);
#else
static inline void _data_copy(void)
{
	/* Do nothing */
}
#endif
FUNC_NORETURN void _Cstart(void);

extern FUNC_NORETURN void _thread_entry(k_thread_entry_t entry,
			  void *p1, void *p2, void *p3);

/* Implemented by architectures. Only called from _setup_new_thread. */
extern void _new_thread(struct k_thread *thread, k_thread_stack_t *pStack,
			size_t stackSize, k_thread_entry_t entry,
			void *p1, void *p2, void *p3,
			int prio, unsigned int options);

extern void _setup_new_thread(struct k_thread *new_thread,
			      k_thread_stack_t *stack, size_t stack_size,
			      k_thread_entry_t entry,
			      void *p1, void *p2, void *p3,
			      int prio, u32_t options);

#ifdef CONFIG_USERSPACE
/**
 * @brief Get the maximum number of partitions for a memory domain
 *
 * A memory domain is a container data structure containing some number of
 * memory partitions, where each partition represents a memory range with
 * access policies.
 *
 * MMU-based systems don't have a limit here, but MPU-based systems will
 * have an upper bound on how many different regions they can manage
 * simultaneously.
 *
 * @return Max number of free regions, or -1 if there is no limit
 */
extern int _arch_mem_domain_max_partitions_get(void);

/**
 * @brief Configure the memory domain of the thread.
 *
 * A memory domain is a container data structure containing some number of
 * memory partitions, where each partition represents a memory range with
 * access policies. This api will configure the appropriate hardware
 * registers to make it work.
 *
 * @param thread Thread which needs to be configured.
 */
extern void _arch_mem_domain_configure(struct k_thread *thread);

/**
 * @brief Remove a partition from the memory domain
 *
 * A memory domain contains multiple partitions and this API provides the
 * freedom to remove a particular partition while keeping others intact.
 * This API will handle any arch/HW specific changes that needs to be done.
 *
 * @param domain The memory domain structure
 * @param partition_id The partition that needs to be deleted
 */
extern void _arch_mem_domain_partition_remove(struct k_mem_domain *domain,
					      u32_t  partition_id);

/**
 * @brief Remove the memory domain
 *
 * A memory domain contains multiple partitions and this API will traverse
 * all these to reset them back to default setting.
 * This API will handle any arch/HW specific changes that needs to be done.
 *
 * @param domain The memory domain structure which needs to be deleted.
 */
extern void _arch_mem_domain_destroy(struct k_mem_domain *domain);
#endif

#ifdef CONFIG_USERSPACE
/**
 * @brief Check memory region permissions
 *
 * Given a memory region, return whether the current memory management hardware
 * configuration would allow a user thread to read/write that region. Used by
 * system calls to validate buffers coming in from userspace.
 *
 * @param addr start address of the buffer
 * @param size the size of the buffer
 * @param write If nonzero, additionally check if the area is writable.
 *	  Otherwise, just check if the memory can be read.
 *
 * @return nonzero if the permissions don't match.
 */
extern int _arch_buffer_validate(void *addr, size_t size, int write);

/**
 * Perform a one-way transition from supervisor to kernel mode.
 *
 * Implementations of this function must do the following:
 * - Reset the thread's stack pointer to a suitable initial value. We do not
 *   need any prior context since this is a one-way operation.
 * - Set up any kernel stack region for the CPU to use during privilege
 *   elevation
 * - Put the CPU in whatever its equivalent of user mode is
 * - Transfer execution to _new_thread() passing along all the supplied
 *   arguments, in user mode.
 *
 * @param Entry point to start executing as a user thread
 * @param p1 1st parameter to user thread
 * @param p2 2nd parameter to user thread
 * @param p3 3rd parameter to user thread
 */
extern FUNC_NORETURN
void _arch_user_mode_enter(k_thread_entry_t user_entry, void *p1, void *p2,
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
extern FUNC_NORETURN void _arch_syscall_oops(void *ssf);

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

extern void _thread_essential_set(void);
extern void _thread_essential_clear(void);

/* clean up when a thread is aborted */

#if defined(CONFIG_THREAD_MONITOR)
extern void _thread_monitor_exit(struct k_thread *thread);
#else
#define _thread_monitor_exit(thread) \
	do {/* nothing */    \
	} while (false)
#endif /* CONFIG_THREAD_MONITOR */

extern void smp_init(void);

extern void smp_timer_init(void);

#ifdef CONFIG_NEWLIB_LIBC
/**
 * @brief Fetch dimentions of newlib heap area for _sbrk()
 *
 * This memory region is used for heap allocations by the newlib C library.
 * If user threads need to have access to this, the results returned can be
 * used to program memory protection hardware appropriately.
 *
 * @param base Pointer to void pointer, filled in with the heap starting
 *             address
 * @param size Pointer to a size_y, filled in with the maximum heap size
 */
extern void z_newlib_get_heap_bounds(void **base, size_t *size);
#endif

extern u32_t z_early_boot_rand32_get(void);

#if CONFIG_STACK_POINTER_RANDOM
extern int z_stack_adjust_initialized;
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_KERNEL_INCLUDE_KERNEL_INTERNAL_H_ */
