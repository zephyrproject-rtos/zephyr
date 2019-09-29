/*
 * Copyright (c) 2019 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_HEAP_SYSMEMPOOL_COMPAT_H_
#define ZEPHYR_INCLUDE_SYS_HEAP_SYSMEMPOOL_COMPAT_H_

#include <kernel.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <sys/mutex.h>
#include <sys/heap-compat-common.h>
#include <sys/sys_heap.h>

/*
 * ============================================================================
 * sys_mem_pool compatibility wrappers
 * ============================================================================
 */

struct sys_mem_pool {
	struct sys_heap heap;
	struct sys_mutex mutex;
};

struct sys_mem_pool_block {
	struct sys_mem_pool *pool;
	u32_t level : 4;
	u32_t block : 28;
};

/**
 * @brief Compatibility wrapper for system memory pool using the heap allocator
 *
 * This pool will not be in an initialized state. You will still need to
 * run sys_mem_pool_init() on it before using any other APIs.
 *
 * @param name Name of the memory pool.
 * @param ignored ignored.
 * @param minsz ignored.
 * @param maxsz Size of the largest blocks in the pool (in bytes).
 * @param nmax Number of maximum sized blocks in the pool.
 * @param align ignored.
 * @param section Destination binary section for pool data
 */
#define SYS_MEM_POOL_DEFINE(name, ignored, minsz, maxsz, nmax, align, section) \
	char __aligned(CHUNK_UNIT) Z_GENERIC_SECTION(section) \
		_heap_buf_##name[SYS_HEAP_BUF_SIZE(maxsz, nmax)]; \
	Z_GENERIC_SECTION(section) struct sys_mem_pool name = { \
		.heap = { \
			.init_mem = _heap_buf_##name, \
			.init_bytes = SYS_HEAP_BUF_SIZE(maxsz, nmax), \
		} \
	}

static inline void sys_compat_mem_pool_init(struct sys_mem_pool *p)
{
	sys_heap_init(&p->heap, p->heap.init_mem, p->heap.init_bytes);
}

void *sys_compat_mem_pool_alloc(struct sys_mem_pool *p, size_t size);

void sys_compat_mem_pool_free(void *ptr);

size_t sys_compat_mem_pool_try_expand_inplace(void *ptr, size_t new_size);

#define sys_mem_pool_init		sys_compat_mem_pool_init
#define sys_mem_pool_alloc		sys_compat_mem_pool_alloc
#define sys_mem_pool_free		sys_compat_mem_pool_free
#define sys_mem_pool_try_expand_inplace	sys_compat_mem_pool_try_expand_inplace

#endif
