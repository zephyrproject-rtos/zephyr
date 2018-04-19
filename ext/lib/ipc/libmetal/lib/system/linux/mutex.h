/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	linux/mutex.h
 * @brief	Linux mutex primitives for libmetal.
 */

#ifndef __METAL_MUTEX__H__
#error "Include metal/mutex.h instead of metal/linux/mutex.h"
#endif

#ifndef __METAL_LINUX_MUTEX__H__
#define __METAL_LINUX_MUTEX__H__

#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>

#include <metal/atomic.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	atomic_int v;
} metal_mutex_t;

/*
 * METAL_MUTEX_INIT - used for initializing an mutex elmenet in a static struct
 * or global
 */
#define METAL_MUTEX_INIT(m) { ATOMIC_VAR_INIT(0) }
/*
 * METAL_MUTEX_DEFINE - used for defining and initializing a global or
 * static singleton mutex
 */
#define METAL_MUTEX_DEFINE(m) metal_mutex_t m = METAL_MUTEX_INIT(m)

static inline int __metal_mutex_cmpxchg(metal_mutex_t *mutex,
					int exp, int val)
{
	atomic_compare_exchange_strong(&mutex->v, (int *)&exp, val);
	return exp;
}

static inline void __metal_mutex_init(metal_mutex_t *mutex)
{
	atomic_store(&mutex->v, 0);
}

static inline void __metal_mutex_deinit(metal_mutex_t *mutex)
{
	(void)mutex;
}

static inline int __metal_mutex_try_acquire(metal_mutex_t *mutex)
{
	int val = 0;
	return atomic_compare_exchange_strong(&mutex->v, &val, 1);
}

static inline void __metal_mutex_acquire(metal_mutex_t *mutex)
{
	int c = 0;

	if (atomic_compare_exchange_strong(&mutex->v, &c, 1))
		return;
	if (c != 2)
		c = atomic_exchange(&mutex->v, 2);
	while (c != 0) {
		syscall(SYS_futex, &mutex->v, FUTEX_WAIT, 2, NULL, NULL, 0);
		c = atomic_exchange(&mutex->v, 2);
	}
}

static inline void __metal_mutex_release(metal_mutex_t *mutex)
{
	if (atomic_fetch_sub(&mutex->v, 1) != 1) {
		atomic_store(&mutex->v, 0);
		syscall(SYS_futex, &mutex->v, FUTEX_WAKE, 1, NULL, NULL, 0);
	}
}

static inline int __metal_mutex_is_acquired(metal_mutex_t *mutex)
{
	return atomic_load(&mutex->v);
}

#ifdef __cplusplus
}
#endif

#endif /* __METAL_LINUX_MUTEX__H__ */
