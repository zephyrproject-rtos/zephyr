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

/* helper type alias for thread control structure */

typedef void (*_thread_entry_t)(void *, void *, void *);

extern void _thread_entry(void (*)(void *, void *, void *),
			  void *, void *, void *);

extern void _new_thread(char *pStack, size_t stackSize,
			void (*pEntry)(void *, void *, void *),
			void *p1, void *p2, void *p3,
			int prio, unsigned options);

/* context switching and scheduling-related routines */

extern unsigned int _Swap(unsigned int);

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
