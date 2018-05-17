/*
 * Copyright (c) 2017, Linaro Limited. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	zephyr/mutex.h
 * @brief	Zephyr mutex primitives for libmetal.
 */

#ifndef __METAL_MUTEX__H__
#error "Include metal/mutex.h instead of metal/zephyr/mutex.h"
#endif

#ifndef __METAL_ZEPHYR_MUTEX__H__
#define __METAL_ZEPHYR_MUTEX__H__

#include <metal/atomic.h>
#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct k_sem metal_mutex_t;

/*
 * METAL_MUTEX_INIT - used for initializing an mutex elmenet in a static struct
 * or global
 */
#define METAL_MUTEX_INIT(m) _K_SEM_INITIALIZER(m, 1, 1)
/*
 * METAL_MUTEX_DEFINE - used for defining and initializing a global or
 * static singleton mutex
 */
#define METAL_MUTEX_DEFINE(m) K_SEM_DEFINE(m, 1, 1)

static inline void __metal_mutex_init(metal_mutex_t *m)
{
	k_sem_init(m, 1, 1);
}

static inline void __metal_mutex_deinit(metal_mutex_t *m)
{
	(void)m;
}

static inline int __metal_mutex_try_acquire(metal_mutex_t *m)
{
        int key = irq_lock(), ret = 1;

        if (m->count) {
                m->count = 0;
                ret = 0;
        }

        irq_unlock(key);

        return ret;
}

static inline int __metal_mutex_is_acquired(metal_mutex_t *m)
{
        int key = irq_lock(), ret;

	ret = m->count;

        irq_unlock(key);

	return ret;
}

static inline void __metal_mutex_acquire(metal_mutex_t *m)
{
	k_sem_take(m, K_FOREVER);
}

static inline void __metal_mutex_release(metal_mutex_t *m)
{
	k_sem_give(m);
}

#ifdef __cplusplus
}
#endif

#endif /* __METAL_ZEPHYR_MUTEX__H__ */
