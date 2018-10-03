/*
 * Copyright (c) 2016, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	freertos/mutex.h
 * @brief	FreeRTOS mutex primitives for libmetal.
 */

#ifndef __METAL_MUTEX__H__
#error "Include metal/mutex.h instead of metal/freertos/mutex.h"
#endif

#ifndef __METAL_FREERTOS_MUTEX__H__
#define __METAL_FREERTOS_MUTEX__H__

#include <metal/atomic.h>
#include <stdlib.h>

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
#define METAL_MUTEX_INIT(m)		{ ATOMIC_VAR_INIT(0) }
/*
 * METAL_MUTEX_DEFINE - used for defining and initializing a global or
 * static singleton mutex
 */
#define METAL_MUTEX_DEFINE(m) metal_mutex_t m = METAL_MUTEX_INIT(m)

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
	return 1 - atomic_flag_test_and_set(&mutex->v);
}

static inline void __metal_mutex_acquire(metal_mutex_t *mutex)
{
	while (atomic_flag_test_and_set(&mutex->v)) {
		;
	}
}

static inline void __metal_mutex_release(metal_mutex_t *mutex)
{
	atomic_flag_clear(&mutex->v);
}

static inline int __metal_mutex_is_acquired(metal_mutex_t *mutex)
{
	return atomic_load(&mutex->v);
}

#ifdef __cplusplus
}
#endif

#endif /* __METAL_FREERTOS_MUTEX__H__ */
