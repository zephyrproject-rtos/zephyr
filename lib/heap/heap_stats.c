/*
 * Copyright (c) 2019,2023 Intel Corporation
 * Copyright (c) 2026 Qualcomm Technologies, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/sys_heap.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "heap.h"

LOG_MODULE_DECLARE(os_heap, CONFIG_SYS_HEAP_LOG_LEVEL);

#ifdef CONFIG_SYS_HEAP_RUNTIME_STATS

int sys_heap_runtime_stats_get(struct sys_heap *heap,
		struct sys_memory_stats *stats)
{
	if ((heap == NULL) || (stats == NULL) || (heap->heap == NULL)) {
		return -EINVAL;
	}

	stats->free_bytes = heap->heap->free_bytes;
	stats->allocated_bytes = heap->heap->allocated_bytes;
	stats->max_allocated_bytes = heap->heap->max_allocated_bytes;

	return 0;
}

int sys_heap_runtime_stats_reset_max(struct sys_heap *heap)
{
	if (heap == NULL) {
		return -EINVAL;
	}

	heap->heap->max_allocated_bytes = heap->heap->allocated_bytes;

	return 0;
}

#endif /* CONFIG_SYS_HEAP_RUNTIME_STATS */

#ifdef CONFIG_SYS_HEAP_THREAD_STATS

void sys_heap_stats_log(struct sys_heap *heap)
{
	struct z_heap *h;
	size_t i, n;
	size_t isr, boot, ovfl;

	if (heap == NULL || heap->heap == NULL) {
		return;
	}

	h = heap->heap;

	if (h->stats == NULL) {
		LOG_WRN("sys_heap %p: no per-thread stats (pool exhausted at init)",
			(void *)h);
		return;
	}

	LOG_INF("sys_heap %p per-thread allocation stats:", (void *)h);

	isr  = (size_t)atomic_get(&h->stats->isr_alloc_bytes);
	boot = (size_t)atomic_get(&h->stats->boot_alloc_bytes);
	ovfl = (size_t)atomic_get(&h->stats->overflow_alloc_bytes);

	if (isr > 0) {
		LOG_INF("  ISR context:         %zu bytes allocated", isr);
	}
	if (boot > 0) {
		LOG_INF("  pre-scheduler boot:  %zu bytes allocated", boot);
	}
	if (ovfl > 0) {
		LOG_INF("  overflow (untracked threads): %zu bytes allocated",
			ovfl);
	}

	n = (size_t)atomic_get(&h->stats->num_threads);

	for (i = 0; i < n; i++) {
		struct z_heap_thread_stat *t = &h->stats->threads[i];

#if defined(CONFIG_THREAD_NAME)
		/*
		 * Refresh the stored name if it was empty at alloc time,
		 * e.g. the thread called k_thread_name_set() after its first
		 * allocation.
		 */
		if (t->name[0] == '\0') {
			const char *live = k_thread_name_get(t->thread);

			if (live != NULL) {
				strncpy(t->name, live, sizeof(t->name) - 1);
				t->name[sizeof(t->name) - 1] = '\0';
			}
		}

		if (t->name[0] != '\0') {
			LOG_INF("  thread %p (%s): %zu bytes allocated",
				(void *)t->thread, t->name, t->total_alloc);
		} else {
			LOG_INF("  thread %p: %zu bytes allocated",
				(void *)t->thread, t->total_alloc);
		}
#else
		LOG_INF("  thread %p: %zu bytes allocated",
			(void *)t->thread, t->total_alloc);
#endif
	}

	if (n == 0 && isr == 0 && boot == 0 && ovfl == 0) {
		LOG_INF("  (no allocations recorded)");
	}
}

#endif /* CONFIG_SYS_HEAP_THREAD_STATS */

#ifdef CONFIG_SYS_HEAP_CALLER_POINTER

void *sys_heap_get_caller(struct sys_heap *heap, void *mem)
{
	struct z_heap *h;
	chunkid_t c;
	uintptr_t base, ptr, heap_end;

	if (heap == NULL || heap->heap == NULL || mem == NULL) {
		return NULL;
	}

	h = heap->heap;
	base = (uintptr_t)chunk_buf(h) + chunk_header_bytes(h);
	ptr = (uintptr_t)mem;
	heap_end = (uintptr_t)chunk_buf(h) + h->end_chunk * CHUNK_UNIT;

	if (ptr < base || ptr >= heap_end) {
		return NULL;
	}

	c = (chunkid_t)((ptr - chunk_header_bytes(h)
			 - (uintptr_t)chunk_buf(h)) / CHUNK_UNIT);

	if (c >= h->end_chunk || !chunk_used(h, c)) {
		return NULL;
	}

	return chunk_trailer(h, c)->caller;
}

#endif /* CONFIG_SYS_HEAP_CALLER_POINTER */
