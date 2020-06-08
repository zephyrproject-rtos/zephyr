/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <string.h>
#include <sys/math_extras.h>

void k_mem_pool_free(struct k_mem_block *block)
{
	k_mem_pool_free_id(&block->id);
}

void *k_mem_pool_malloc(struct k_mem_pool *pool, size_t size)
{
	struct k_mem_block block;

	/*
	 * get a block large enough to hold an initial (hidden) block
	 * descriptor, as well as the space the caller requested
	 */
	if (size_add_overflow(size, WB_UP(sizeof(struct k_mem_block_id)),
			      &size)) {
		return NULL;
	}
	if (k_mem_pool_alloc(pool, &block, size, K_NO_WAIT) != 0) {
		return NULL;
	}

	/* save the block descriptor info at the start of the actual block */
	(void)memcpy(block.data, &block.id, sizeof(struct k_mem_block_id));

	/* return address of the user area part of the block to the caller */
	return (char *)block.data + WB_UP(sizeof(struct k_mem_block_id));
}

void k_free(void *ptr)
{
	if (ptr != NULL) {
		/* point to hidden block descriptor at start of block */
		ptr = (char *)ptr - WB_UP(sizeof(struct k_mem_block_id));

		/* return block to the heap memory pool */
		k_mem_pool_free_id(ptr);
	}
}

#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)

/*
 * Heap is defined using HEAP_MEM_POOL_SIZE configuration option.
 *
 * This module defines the heap memory pool and the _HEAP_MEM_POOL symbol
 * that has the address of the associated memory pool struct.
 */

K_MEM_POOL_DEFINE(_heap_mem_pool, CONFIG_HEAP_MEM_POOL_MIN_SIZE,
		  CONFIG_HEAP_MEM_POOL_SIZE, 1, 4);
#define _HEAP_MEM_POOL (&_heap_mem_pool)

void *k_malloc(size_t size)
{
	return k_mem_pool_malloc(_HEAP_MEM_POOL, size);
}

void *k_calloc(size_t nmemb, size_t size)
{
	void *ret;
	size_t bounds;

	if (size_mul_overflow(nmemb, size, &bounds)) {
		return NULL;
	}

	ret = k_malloc(bounds);
	if (ret != NULL) {
		(void)memset(ret, 0, bounds);
	}
	return ret;
}

void k_thread_system_pool_assign(struct k_thread *thread)
{
	thread->resource_pool = _HEAP_MEM_POOL;
}
#else
#define _HEAP_MEM_POOL	NULL
#endif

void *z_thread_malloc(size_t size)
{
	void *ret;
	struct k_mem_pool *pool;

	if (k_is_in_isr()) {
		pool = _HEAP_MEM_POOL;
	} else {
		pool = _current->resource_pool;
	}

	if (pool) {
		ret = k_mem_pool_malloc(pool, size);
	} else {
		ret = NULL;
	}

	return ret;
}
