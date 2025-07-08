/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Real-Time IO pool API for maintaining a pool for RTIO contexts
 */

#ifndef ZEPHYR_INCLUDE_RTIO_POOL_H_
#define ZEPHYR_INCLUDE_RTIO_POOL_H_

#include "zephyr/kernel.h"
#include <zephyr/sys/atomic.h>
#include <zephyr/rtio/rtio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RTIO Pool
 * @defgroup rtio_pool RTIO Pool API
 * @ingroup rtio
 * @{
 */

/**
 * @brief Pool of RTIO contexts to use with dynamically created threads
 */
struct rtio_pool {
	/** Size of the pool */
	size_t pool_size;

	/** Array containing contexts of the pool */
	struct rtio **contexts;

	/** Atomic bitmap to signal a member is used/unused */
	atomic_t *used;
};

/**
 * @brief Obtain an RTIO context from a pool
 */
__syscall struct rtio *rtio_pool_acquire(struct rtio_pool *pool);

static inline struct rtio *z_impl_rtio_pool_acquire(struct rtio_pool *pool)
{
	struct rtio *r = NULL;

	for (size_t i = 0; i < pool->pool_size; i++) {
		if (atomic_test_and_set_bit(pool->used, i) == 0) {
			r = pool->contexts[i];
			break;
		}
	}

	k_object_access_grant(r, k_current_get());

	return r;
}

/**
 * @brief Return an RTIO context to a pool
 */
__syscall void rtio_pool_release(struct rtio_pool *pool, struct rtio *r);

static inline void z_impl_rtio_pool_release(struct rtio_pool *pool, struct rtio *r)
{
	k_object_access_revoke(r, k_current_get());

	for (size_t i = 0; i < pool->pool_size; i++) {
		if (pool->contexts[i] == r) {
			atomic_clear_bit(pool->used, i);
			break;
		}
	}
}

/* clang-format off */

/** @cond ignore */

#define Z_RTIO_POOL_NAME_N(n, name)                                             \
	name##_##n

#define Z_RTIO_POOL_DEFINE_N(n, name, sq_sz, cq_sz)				\
	RTIO_DEFINE(Z_RTIO_POOL_NAME_N(n, name), sq_sz, cq_sz)

#define Z_RTIO_POOL_REF_N(n, name)                                              \
	&Z_RTIO_POOL_NAME_N(n, name)

/** @endcond */

/**
 * @brief Statically define and initialize a pool of RTIO contexts
 *
 * @param name Name of the RTIO pool
 * @param pool_sz Number of RTIO contexts to allocate in the pool
 * @param sq_sz Size of the submission queue entry pool per context
 * @param cq_sz Size of the completion queue entry pool per context
 */
#define RTIO_POOL_DEFINE(name, pool_sz, sq_sz, cq_sz)				\
	LISTIFY(pool_sz, Z_RTIO_POOL_DEFINE_N, (;), name, sq_sz, cq_sz);        \
	static struct rtio * name##_contexts[] = {                              \
		LISTIFY(pool_sz, Z_RTIO_POOL_REF_N, (,), name)                  \
	};                                                                      \
	ATOMIC_DEFINE(name##_used, pool_sz);                                    \
	STRUCT_SECTION_ITERABLE(rtio_pool, name) = {                            \
		.pool_size = pool_sz,                                           \
		.contexts = name##_contexts,                                    \
		.used = name##_used,                                            \
	}

/* clang-format on */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/pool.h>

#endif /* ZEPHYR_INCLUDE_RTIO_POOL_H_ */
