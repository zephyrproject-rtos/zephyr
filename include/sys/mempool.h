/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_MEMPOOL_H_
#define ZEPHYR_INCLUDE_SYS_MEMPOOL_H_

#include <kernel.h>
#include <sys/mempool_base.h>
#include <sys/mutex.h>

struct sys_mem_pool {
	struct sys_mem_pool_base base;
	struct sys_mutex mutex;
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
 * @param ignored ignored, any value
 * @param minsz Size of the smallest blocks in the pool (in bytes).
 * @param maxsz Size of the largest blocks in the pool (in bytes).
 * @param nmax Number of maximum sized blocks in the pool.
 * @param align Alignment of the pool's buffer (power of 2).
 * @param section Destination binary section for pool data
 */
#define SYS_MEM_POOL_DEFINE(name, ignored, minsz, maxsz, nmax, align, section) \
	char __aligned(WB_UP(align)) Z_GENERIC_SECTION(section)		\
		_mpool_buf_##name[WB_UP(maxsz) * nmax			\
				  + _MPOOL_BITS_SIZE(maxsz, minsz, nmax)]; \
	struct sys_mem_pool_lvl Z_GENERIC_SECTION(section)		\
		_mpool_lvls_##name[Z_MPOOL_LVLS(maxsz, minsz)];		\
	Z_GENERIC_SECTION(section) struct sys_mem_pool name = {		\
		.base = {						\
			.buf = _mpool_buf_##name,			\
			.max_sz = WB_UP(maxsz),				\
			.n_max = nmax,					\
			.n_levels = Z_MPOOL_LVLS(maxsz, minsz),		\
			.levels = _mpool_lvls_##name,			\
			.flags = SYS_MEM_POOL_USER			\
		}							\
	};								\
	BUILD_ASSERT(WB_UP(maxsz) >= _MPOOL_MINBLK)

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
	z_sys_mem_pool_base_init(&p->base);
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

/**
 * @brief Try to perform in-place expansion of memory allocated from a pool
 *
 * Return 0 if memory previously allocated by sys_mem_pool_alloc()
 * can accommodate a new size, otherwise return the size of data that
 * needs to be copied over to new memory.
 *
 * @param ptr Pointer to previously allocated memory
 * @param new_size New size requested for the memory block
 * @return A 0 if OK, or size of data to copy elsewhere
 */
size_t sys_mem_pool_try_expand_inplace(void *ptr, size_t new_size);

#endif
