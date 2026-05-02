/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "os_wrapper.h"
#include <zephyr/sys/math_extras.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(os_if_memory);

#if (K_HEAP_MEM_POOL_SIZE > 0)
#define _SYSTEM_HEAP (&_system_heap)
extern struct sys_heap _system_heap;
#endif

void rtos_mem_init(void)
{
	/* Zephyr initializes the system heap automatically. Nothing to do. */
}

void rtos_mem_free(void *pbuf)
{
	if (pbuf == NULL) {
		return;
	}
	k_free(pbuf);
	pbuf = NULL;
}

void *rtos_mem_malloc(uint32_t size)
{
#if (K_HEAP_MEM_POOL_SIZE > 0)
	return k_aligned_alloc(CACHE_LINE_SIZE, CACHE_LINE_ALIGNMENT(size));
#else
	LOG_ERR("k_aligned_alloc not support.");
	return NULL;
#endif
}

void *rtos_mem_zmalloc(uint32_t size)
{
	void *pbuf = NULL;

	pbuf = rtos_mem_malloc(size);
	if (pbuf != NULL) {
		memset(pbuf, 0, size);
	}

	return pbuf;
}

void *rtos_mem_calloc(uint32_t elementNum, uint32_t elementSize)
{
	uint32_t sz = elementNum * elementSize;

	return rtos_mem_zmalloc(sz);
}

void *rtos_mem_realloc(void *pbuf, uint32_t size)
{
	struct k_heap *heap, **heap_ref;
	void *ret;

	if (size == 0) {
		rtos_mem_free(pbuf);
		return NULL;
	}
	if (pbuf == NULL) {
		return rtos_mem_malloc(size);
	}

	size = CACHE_LINE_ALIGNMENT(size);

	heap_ref = pbuf;
	pbuf = --heap_ref;
	heap = *heap_ref;
	if (size_add_overflow(size, sizeof(heap_ref), &size)) {
		return NULL;
	}

	k_spinlock_key_t key = k_spin_lock(&heap->lock);

	ret = sys_heap_aligned_realloc(&heap->heap, pbuf, CACHE_LINE_SIZE, size);
	k_spin_unlock(&heap->lock, key);

	if (ret != NULL) {
		heap_ref = ret;
		ret = ++heap_ref;
	}

	return ret;
}

uint32_t rtos_mem_get_free_heap_size(void)
{
	uint32_t size = 0;

#ifdef CONFIG_SYS_HEAP_RUNTIME_STATS
	struct sys_memory_stats stats;

	sys_heap_runtime_stats_get(_SYSTEM_HEAP, &stats);
	size = stats.free_bytes;
#else
	LOG_ERR("Not Support");
#endif
	return size;
}

uint32_t rtos_mem_get_minimum_ever_free_heap_size(void)
{
	uint32_t size = 0;

#ifdef CONFIG_SYS_HEAP_RUNTIME_STATS
	struct sys_memory_stats stats;

	sys_heap_runtime_stats_get(_SYSTEM_HEAP, &stats);
	size = K_HEAP_MEM_POOL_SIZE - stats.max_allocated_bytes;
#else
	LOG_ERR("Not Support");
#endif
	return size;
}
