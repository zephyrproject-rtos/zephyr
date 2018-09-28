/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MISC_MEMPOOL_H_
#define ZEPHYR_INCLUDE_MISC_MEMPOOL_H_

#include <kernel.h>
#include <misc/mempool_base.h>

struct sys_mem_pool {
	struct sys_mem_pool_base base;
	struct k_mutex *mutex;
};

struct sys_mem_pool_block {
	struct sys_mem_pool *pool;
	u32_t level : 4;
	u32_t block : 28;
};

/**
 * @brief Statically define system memory pool
 *
 * The memory pool's buffer contains @a n_max blocks that are @a max_size bytes
 * long. The memory pool allows blocks to be repeatedly partitioned into
 * quarters, down to blocks of @a min_size bytes long. The buffer is aligned
 * to a @a align -byte boundary.
 *
 * If the pool is to be accessed outside the module where it is defined, it
 * can be declared via
 *
 * @code extern struct sys_mem_pool <name>; @endcode
 *
 * This pool will not be in an initialized state. You will still need to
 * run sys_mem_pool_init() on it before using any other APIs.
 *
 * @param name Name of the memory pool.
 * @param kmutex Pointer to an initialized k_mutex object, used for
 *		 synchronization, declared with K_MUTEX_DEFINE().
 * @param minsz Size of the smallest blocks in the pool (in bytes).
 * @param maxsz Size of the largest blocks in the pool (in bytes).
 * @param nmax Number of maximum sized blocks in the pool.
 * @param align Alignment of the pool's buffer (power of 2).
 * @param section Destination binary section for pool data
 */
#define SYS_MEM_POOL_DEFINE(name, kmutex, minsz, maxsz, nmax, align, section) \
	char __aligned(align) _GENERIC_SECTION(section)			\
		_mpool_buf_##name[_ALIGN4(maxsz * nmax)			\
				  + _MPOOL_BITS_SIZE(maxsz, minsz, nmax)]; \
	struct sys_mem_pool_lvl _GENERIC_SECTION(section)		\
		_mpool_lvls_##name[_MPOOL_LVLS(maxsz, minsz)];		\
	_GENERIC_SECTION(section) struct sys_mem_pool name = {		\
		.base = {						\
			.buf = _mpool_buf_##name,			\
			.max_sz = maxsz,				\
			.n_max = nmax,					\
			.n_levels = _MPOOL_LVLS(maxsz, minsz),		\
			.levels = _mpool_lvls_##name,			\
			.flags = SYS_MEM_POOL_USER			\
		},							\
		.mutex = kmutex,					\
	}

/**
 * @brief Initialize a memory pool
 *
 * This is intended to complete initialization of memory pools that have been
 * declared with SYS_MEM_POOL_DEFINE().
 *
 * @param p Memory pool to initialize
 */
static inline void sys_mem_pool_init(struct sys_mem_pool *p)
{
	_sys_mem_pool_base_init(&p->base);
}

/**
 * @brief Allocate a block of memory
 *
 * Allocate a chunk of memory from a memory pool. This cannot be called from
 * interrupt context.
 *
 * @param p Address of the memory pool
 * @param size Requested size of the memory block
 * @return A pointer to the requested memory, or NULL if none is available
 */
void *sys_mem_pool_alloc(struct sys_mem_pool *p, size_t size);

/**
 * @brief Free memory allocated from a memory pool
 *
 * Free memory previously allocated by sys_mem_pool_alloc().
 * It is safe to pass NULL to this function, in which case it is a no-op.
 *
 * @param ptr Pointer to previously allocated memory
 */
void sys_mem_pool_free(void *ptr);

#endif
