/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <string.h>
#include <sys/math_extras.h>
#include <sys/util.h>

static void *z_heap_aligned_alloc(struct k_heap *heap, size_t align, size_t size)
{
	uint8_t *mem;
	struct k_heap **heap_ref;
	size_t excess = MAX(sizeof(struct k_heap *), align);

	/*
	 * get a block large enough to hold an initial (aligned and hidden) heap
	 * pointer, as well as the space the caller requested
	 */
	if (size_add_overflow(size, excess, &size)) {
		return NULL;
	}

	mem = k_heap_aligned_alloc(heap, align, size, K_NO_WAIT);
	if (mem == NULL) {
		return NULL;
	}

	/* create (void *) values in the excess equal to (void *) -1 */
	memset(mem, 0xff, excess);
	heap_ref = (struct k_heap **)mem;
	*heap_ref = heap;

	/* return address of the user area part of the block to the caller */
	return mem + excess;
}

void k_free(void *ptr)
{
	struct k_heap **heap_ref;

	if (ptr != NULL) {
		for (heap_ref = &((struct k_heap **)ptr)[-1];
			*heap_ref == (struct k_heap *)-1; --heap_ref) {
			/* no-op */
		}

		ptr = (uint8_t *)heap_ref;
		k_heap_free(*heap_ref, ptr);
	}
}

#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)

K_HEAP_DEFINE(_system_heap, CONFIG_HEAP_MEM_POOL_SIZE);
#define _SYSTEM_HEAP (&_system_heap)

void *k_aligned_alloc(size_t align, size_t size)
{
	__ASSERT(align / sizeof(void *) >= 1
		&& (align % sizeof(void *)) == 0,
		"align must be a multiple of sizeof(void *)");

	__ASSERT((align & (align - 1)) == 0,
		"align must be a power of 2");

	return z_heap_aligned_alloc(_SYSTEM_HEAP, align, size);
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

void *z_thread_aligned_alloc(size_t align, size_t size)
{
	void *ret;
	struct k_heap *heap;

	if (k_is_in_isr()) {
		heap = _SYSTEM_HEAP;
	} else {
		heap = _current->resource_pool;
	}

	if (heap) {
		ret = z_heap_aligned_alloc(heap, align, size);
	} else {
		ret = NULL;
	}

	return ret;
}
