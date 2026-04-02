/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/sync_heap.h>

static inline int sys_sync_lock(struct sys_sync_heap *heap)
{
	return k_mutex_lock(heap->lock, K_FOREVER);
}

static inline void sys_sync_unlock(struct sys_sync_heap *heap)
{
	(void)k_mutex_unlock(heap->lock);
}

int sys_sync_heap_init(struct sys_sync_heap *heap, void *mem, size_t bytes)
{
	sys_heap_init(&heap->heap, mem, bytes);
	heap->lock = k_object_alloc(K_OBJ_MUTEX);
	return heap->lock != NULL ? k_mutex_init(heap->lock) : -ENOMEM;
}

void *sys_sync_heap_alloc(struct sys_sync_heap *heap, size_t bytes)
{
	void *ptr = NULL;

	if (sys_sync_lock(heap) == 0) {
		ptr = sys_heap_alloc(&heap->heap, bytes);
		sys_sync_unlock(heap);
	}

	return ptr;
}

void *sys_sync_heap_aligned_alloc(struct sys_sync_heap *heap, size_t align, size_t bytes)
{
	void *ptr = NULL;

	if (sys_sync_lock(heap) == 0) {
		ptr = sys_heap_aligned_alloc(&heap->heap, align, bytes);
		sys_sync_unlock(heap);
	}

	return ptr;
}

void *sys_sync_heap_noalign_alloc(struct sys_sync_heap *heap, size_t align, size_t bytes)
{
	void *ptr = NULL;

	if (sys_sync_lock(heap) == 0) {
		ptr = sys_heap_noalign_alloc(&heap->heap, align, bytes);
		sys_sync_unlock(heap);
	}

	return ptr;
}

void sys_sync_heap_free(struct sys_sync_heap *heap, void *mem)
{
	int lock_ret = sys_sync_lock(heap);

	__ASSERT(lock_ret == 0, "Could not lock heap");
	(void)lock_ret;

	sys_heap_free(&heap->heap, mem);
	sys_sync_unlock(heap);
}

void *sys_sync_heap_realloc(struct sys_sync_heap *heap, void *ptr, size_t bytes)
{
	void *new_ptr = NULL;

	if (sys_sync_lock(heap) == 0) {
		new_ptr = sys_heap_realloc(&heap->heap, ptr, bytes);
		sys_sync_unlock(heap);
	}

	return new_ptr;
}

void *sys_sync_heap_aligned_realloc(struct sys_sync_heap *heap, void *ptr, size_t align,
				    size_t bytes)
{
	void *new_ptr = NULL;

	if (sys_sync_lock(heap) == 0) {
		new_ptr = sys_heap_aligned_realloc(&heap->heap, ptr, align, bytes);
		sys_sync_unlock(heap);
	}

	return new_ptr;
}

int sys_sync_heap_runtime_stats_get(struct sys_sync_heap *heap, struct sys_memory_stats *stats)
{
	int ret = -EINVAL;

	if (sys_sync_lock(heap) == 0) {
		ret = sys_heap_runtime_stats_get(&heap->heap, stats);
		sys_sync_unlock(heap);
	}

	return ret;
}
