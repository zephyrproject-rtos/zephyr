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

extern void _new_thread(struct k_thread *thread, k_thread_stack_t pStack,
			size_t stackSize, k_thread_entry_t entry,
			void *p1, void *p2, void *p3,
			int prio, unsigned int options);

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
#endif /* CONFIG_USERSPACE */

/* set and clear essential fiber/task flag */

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
