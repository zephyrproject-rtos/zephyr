/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <string.h>
#include <sys/math_extras.h>

void z_mem_pool_free(struct k_mem_block *block)
{
	z_mem_pool_free_id(&block->id);
}

void *z_heap_malloc(struct k_heap *heap, size_t size)
{
	/*
	 * get a block large enough to hold an initial (hidden) heap
	 * pointer, as well as the space the caller requested
	 */
	if (size_add_overflow(size, sizeof(struct k_heap *),
			      &size)) {
		return NULL;
	}

	struct k_heap **blk = k_heap_alloc(heap, size, K_NO_WAIT);

	if (blk == NULL) {
		return NULL;
	}

	blk[0] = heap;

	/* return address of the user area part of the block to the caller */
	return (char *)&blk[1];

}

void *z_mem_pool_malloc(struct k_mem_pool *pool, size_t size)
{
	return z_heap_malloc(pool->heap, size);
}

void k_free(void *ptr)
{
	if (ptr != NULL) {
		struct k_heap **blk = &((struct k_heap **)ptr)[-1];
		struct k_heap *heap = *blk;

		k_heap_free(heap, blk);
	}
}

#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)

K_HEAP_DEFINE(_system_heap, CONFIG_HEAP_MEM_POOL_SIZE);
#define _SYSTEM_HEAP (&_system_heap)

void *k_malloc(size_t size)
{
	return z_heap_malloc(_SYSTEM_HEAP, size);
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
	thread->resource_pool = _SYSTEM_HEAP;
}
#else
#define _SYSTEM_HEAP	NULL
#endif

void *z_thread_malloc(size_t size)
{
	void *ret;
	struct k_heap *heap;

	if (k_is_in_isr()) {
		heap = _SYSTEM_HEAP;
	} else {
		heap = _current->resource_pool;
	}

	if (heap) {
		ret = z_heap_malloc(heap, size);
	} else {
		ret = NULL;
	}

	return ret;
}
