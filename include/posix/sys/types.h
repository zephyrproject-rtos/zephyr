/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __POSIX_TYPES_H__
#define __POSIX_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

#include_next <sys/types.h>

#ifdef CONFIG_PTHREAD_IPC
#include <kernel.h>

/* Thread attributes */
typedef struct pthread_attr_t {
	int priority;
	void *stack;
	size_t stacksize;
	u32_t flags;
	u32_t delayedstart;
	u32_t schedpolicy;
	s32_t detachstate;
	u32_t initialized;
} pthread_attr_t;

typedef void *pthread_t;

/* Mutex */
typedef struct pthread_mutex {
	struct k_sem *sem;
} pthread_mutex_t;

typedef struct pthread_mutexattr {
	int unused;
} pthread_mutexattr_t;

/* Condition variables */
typedef struct pthread_cond {
	_wait_q_t wait_q;
} pthread_cond_t;

typedef struct pthread_condattr {
	int unused;
} pthread_condattr_t;

/* Barrier */
typedef struct pthread_barrier {
	_wait_q_t wait_q;
	int max;
	int count;
} pthread_barrier_t;

typedef struct pthread_barrierattr {
	int unused;
} pthread_barrierattr_t;

/* time related attributes */
#ifndef CONFIG_NEWLIB_LIBC
typedef u32_t clockid_t;
#endif /*CONFIG_NEWLIB_LIBC */
typedef unsigned long useconds_t;

#endif /* CONFIG_PTHREAD_IPC */

#ifdef __cplusplus
}
#endif

#endif	/* __POSIX_TYPES_H__ */
