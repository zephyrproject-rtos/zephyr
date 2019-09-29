/*
 * Copyright (c) 2019 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_HEAP_KMEMPOOL_COMPAT_H_
#define ZEPHYR_INCLUDE_SYS_HEAP_KMEMPOOL_COMPAT_H_

/*
 * This whole wrapper business is of course a hack. And so is the following
 * #ifndef to work around situations where sys_heap.h is included first.
 */
#ifndef ZEPHYR_INCLUDE_SYS_SYS_HEAP_H_

#include <sys/heap-compat-common.h>
#include <sys/sys_heap.h>

/*
 * ============================================================================
 * k_mem_pool compatibility wrappers
 * ============================================================================
 */

#define _MPOOL_MINBLK 1

struct k_mem_pool {
	struct sys_heap base;
	_wait_q_t wait_q;
};

/**
 * @brief Wrapper to statically define and initialize a memory pool.
 *
 * @param name Name of the memory pool.
 * @param minsz ignored.
 * @param maxsz Size of the largest blocks in the pool (in bytes).
 * @param nmax Number of maximum sized blocks in the pool.
 * @param align Ignored.
 */
#define K_MEM_POOL_DEFINE(name, minsz, maxsz, nmax, align) \
	char __aligned(CHUNK_UNIT) \
		_heap_buf_##name[SYS_HEAP_BUF_SIZE(maxsz, nmax)]; \
	Z_STRUCT_SECTION_ITERABLE(k_mem_pool, name) = { \
		.base = { \
			.init_mem = &_heap_buf_##name, \
			.init_bytes = SYS_HEAP_BUF_SIZE(maxsz, nmax), \
		}, \
	}

static inline void z_sys_compat_mem_pool_base_init(struct sys_heap *base)
{
	sys_heap_init(base, base->init_mem, base->init_bytes);
}

int z_sys_compat_mem_pool_block_alloc(struct sys_heap *p, size_t size,
				u32_t *level_p, u32_t *block_p, void **data_p);
void z_sys_compat_mem_pool_block_free(struct sys_heap *p, u32_t level,
				      u32_t block);

#define z_sys_mem_pool_base_init	z_sys_compat_mem_pool_base_init
#define z_sys_mem_pool_block_alloc	z_sys_compat_mem_pool_block_alloc
#define z_sys_mem_pool_block_free	z_sys_compat_mem_pool_block_free

#endif
#endif
