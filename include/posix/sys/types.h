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

#ifdef CONFIG_NEWLIB_LIBC
#include_next <sys/types.h>
#endif	/* CONFIG_NEWLIB_LIBC */

#ifdef CONFIG_PTHREAD_IPC
/* Mutex */
typedef struct pthread_mutex {
	struct k_sem *sem;
} pthread_mutex_t;

typedef struct pthread_mutexattr {
	int unused;
} pthread_mutexattr_t;

/* Confition Variables */
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

#endif /* CONFIG_PTHREAD_IPC */

#ifdef __cplusplus
}
#endif

#endif	/* __POSIX_TYPES_H__ */
