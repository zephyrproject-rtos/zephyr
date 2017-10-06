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

#ifndef _NANO_INTERNAL__H_
#define _NANO_INTERNAL__H_

#include <kernel.h>

#define K_NUM_PRIORITIES \
	(CONFIG_NUM_COOP_PRIORITIES + CONFIG_NUM_PREEMPT_PRIORITIES + 1)

#define K_NUM_PRIO_BITMAPS ((K_NUM_PRIORITIES + 31) >> 5)

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

/* context switching and scheduling-related routines */

extern unsigned int __swap(unsigned int key);

#ifdef CONFIG_TIMESLICING
extern void _update_time_slice_before_swap(void);
#endif

#ifdef CONFIG_STACK_SENTINEL
extern void _check_stack_sentinel(void);
#endif

static inline unsigned int _Swap(unsigned int key)
{

#ifdef CONFIG_STACK_SENTINEL
	_check_stack_sentinel();
#endif
#ifdef CONFIG_TIMESLICING
	_update_time_slice_before_swap();
#endif

	return __swap(key);
}

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
#endif /* CONFIG_USERSPACE */

/* set and clear essential thread flag */

extern void _thread_essential_set(void);
extern void _thread_essential_clear(void);

/* clean up when a thread is aborted */

#if defined(CONFIG_THREAD_MONITOR)
extern void _thread_monitor_exit(struct k_thread *thread);
#else
#define _thread_monitor_exit(thread) \
	do {/* nothing */    \
	} while (0)
#endif /* CONFIG_THREAD_MONITOR */

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* _NANO_INTERNAL__H_ */
