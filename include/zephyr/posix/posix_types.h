/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_POSIX_TYPES_H_
#define ZEPHYR_INCLUDE_POSIX_TYPES_H_

#if !(defined(CONFIG_ARCH_POSIX) && defined(CONFIG_EXTERNAL_LIBC))
#include <sys/types.h>
#endif

#include <zephyr/kernel.h>
#include <zephyr/posix/sys/features.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int pid_t;

#ifndef __useconds_t_defined
typedef unsigned long useconds_t;
#endif

/* time related attributes */
#if !defined(CONFIG_NEWLIB_LIBC) && !defined(CONFIG_ARCMWDT_LIBC)
#ifndef __clockid_t_defined
typedef uint32_t clockid_t;
#endif
#endif /* !CONFIG_NEWLIB_LIBC && !CONFIG_ARCMWDT_LIBC */
#ifndef __timer_t_defined
typedef unsigned long timer_t;
#endif

/* Semaphore */
typedef struct k_sem sem_t;

#ifdef CONFIG_NEWLIB_LIBC
#include <sys/_pthreadtypes.h>

struct pthread_once {
	bool flag;
};

#else

#if defined(_POSIX_THREADS)

/* Thread attributes */
typedef struct pthread_attr {
	void *stack;
	uint32_t details[2];
} pthread_attr_t;

typedef uint32_t pthread_t;
typedef uint32_t pthread_cond_t;
typedef uint32_t pthread_key_t;
typedef uint32_t pthread_mutex_t;

typedef struct pthread_mutexattr {
	unsigned char type: 2;
	bool initialized: 1;
} pthread_mutexattr_t;

typedef struct pthread_condattr {
	clockid_t clock;
} pthread_condattr_t;

typedef struct pthread_once {
	bool flag;
} pthread_once_t;

#endif /* defined(_POSIX_THREADS) */

#if defined(_POSIX_SPIN_LOCKS)

typedef uint32_t pthread_spinlock_t;

#endif /* defined(_POSIX_SPIN_LOCKS) */

#if defined(_POSIX_BARRIERS)

typedef uint32_t pthread_barrier_t;
typedef struct pthread_barrierattr {
	int pshared;
} pthread_barrierattr_t;

#endif /* _POSIX_BARRIERS */

#if defined(_POSIX_READER_WRITER_LOCKS)

typedef uint32_t pthread_rwlockattr_t;
typedef uint32_t pthread_rwlock_t;

#endif /* defined(_POSIX_READER_WRITER_LOCKS) */

#endif /* CONFIG_NEWLIB_LIBC */

#ifdef __cplusplus
}
#endif

#endif	/* ZEPHYR_INCLUDE_POSIX_TYPES_H_ */
