/*
 * Copyright (c) 2018, Pinecone Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	nuttx/mutex.h
 * @brief	NuttX mutex primitives for libmetal.
 */

#ifndef __METAL_MUTEX__H__
#error "Include metal/mutex.h instead of metal/nuttx/mutex.h"
#endif

#ifndef __METAL_NUTTX_MUTEX__H__
#define __METAL_NUTTX_MUTEX__H__

#include <nuttx/mutex.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef mutex_t metal_mutex_t;

/*
 * METAL_MUTEX_INIT - used for initializing an mutex elmenet in a static struct
 * or global
 */
#define METAL_MUTEX_INIT(m) MUTEX_INITIALIZER
/*
 * METAL_MUTEX_DEFINE - used for defining and initializing a global or
 * static singleton mutex
 */
#define METAL_MUTEX_DEFINE(m) metal_mutex_t m = MUTEX_INITIALIZER

static inline void __metal_mutex_init(metal_mutex_t *mutex)
{
	nxmutex_init(mutex);
}

static inline void __metal_mutex_deinit(metal_mutex_t *mutex)
{
	nxmutex_destroy(mutex);
}

static inline int __metal_mutex_try_acquire(metal_mutex_t *mutex)
{
	return nxmutex_trylock(mutex);
}

static inline void __metal_mutex_acquire(metal_mutex_t *mutex)
{
	nxmutex_lock(mutex);
}

static inline void __metal_mutex_release(metal_mutex_t *mutex)
{
	nxmutex_unlock(mutex);
}

static inline int __metal_mutex_is_acquired(metal_mutex_t *mutex)
{
	return nxmutex_is_locked(mutex);
}

#ifdef __cplusplus
}
#endif

#endif /* __METAL_NUTTX_MUTEX__H__ */
