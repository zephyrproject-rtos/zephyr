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
#include <string.h>
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

static struct z_heap_stats z_heap_stats_pool[CONFIG_SYS_HEAP_STATS_TRACKED_HEAPS];
static const struct sys_heap *z_heap_stats_heaps[CONFIG_SYS_HEAP_STATS_TRACKED_HEAPS];
static int z_heap_stats_count;

static struct k_thread *heap_stats_current_thread(void)
{
	if (k_is_in_isr()) {
		return NULL;
	}
	if (k_is_pre_kernel()) {
		return Z_HEAP_BOOT_SENTINEL;
	}
	return k_current_get();
}

static void heap_stats_add(struct z_heap_stats *s,
			   struct k_thread *thread, size_t usable_bytes)
{
	struct z_heap_thread_stat *e;
#if defined(CONFIG_THREAD_NAME)
	const char *name;
#endif
	size_t i;

	for (i = 0; i < s->num_threads; i++) {
		if (s->threads[i].thread == thread) {
			s->threads[i].total_alloc += usable_bytes;
#if defined(CONFIG_THREAD_NAME)
			if (s->threads[i].name[0] == '\0') {
				name = k_thread_name_get(thread);
				if (name != NULL) {
					strncpy(s->threads[i].name, name,
						sizeof(s->threads[i].name) - 1);
					s->threads[i].name[sizeof(s->threads[i].name) - 1] = '\0';
				}
			}
#endif
			return;
		}
	}

	if (s->num_threads >= CONFIG_SYS_HEAP_STATS_MAX_TRACKED_THREADS) {
		s->overflow_alloc_bytes += usable_bytes;
		return;
	}

	e = &s->threads[s->num_threads];
	e->thread = thread;
	e->total_alloc = usable_bytes;

#if defined(CONFIG_THREAD_NAME)
	name = k_thread_name_get(thread);
	if (name != NULL) {
		strncpy(e->name, name, sizeof(e->name) - 1);
		e->name[sizeof(e->name) - 1] = '\0';
	} else {
		e->name[0] = '\0';
	}
#endif

	s->num_threads++;
}

static void heap_stats_sub(struct z_heap_stats *s,
			   struct k_thread *thread, size_t usable_bytes)
{
	size_t i;

	for (i = 0; i < s->num_threads; i++) {
		if (s->threads[i].thread == thread) {
			__ASSERT(s->threads[i].total_alloc >= usable_bytes,
				 "heap stats underflow for thread %p", (void *)thread);
			s->threads[i].total_alloc -=
				MIN(usable_bytes, s->threads[i].total_alloc);
			return;
		}
	}

	/*
	 * Thread not found: either the table was full at alloc time (bytes are
	 * in overflow_alloc_bytes) or the TCB was recycled between alloc/free.
	 */
	__ASSERT(s->overflow_alloc_bytes >= usable_bytes,
		 "overflow_alloc_bytes underflow");
	s->overflow_alloc_bytes -= MIN(usable_bytes, s->overflow_alloc_bytes);
}

static void heap_stats_update(struct z_heap_stats *s,
			      struct k_thread *old_owner, size_t old_bytes,
			      struct k_thread *new_owner, size_t new_bytes)
{
	if (old_bytes > 0) {
		if (old_owner == NULL) {
			__ASSERT(s->isr_alloc_bytes >= old_bytes,
				 "isr_alloc_bytes underflow");
			s->isr_alloc_bytes -= MIN(old_bytes, s->isr_alloc_bytes);
		} else if (old_owner == Z_HEAP_BOOT_SENTINEL) {
			__ASSERT(s->boot_alloc_bytes >= old_bytes,
				 "boot_alloc_bytes underflow");
			s->boot_alloc_bytes -= MIN(old_bytes, s->boot_alloc_bytes);
		} else {
			heap_stats_sub(s, old_owner, old_bytes);
		}
	}

	if (new_bytes == 0) {
		return;
	}
	if (new_owner == NULL) {
		s->isr_alloc_bytes += new_bytes;
	} else if (new_owner == Z_HEAP_BOOT_SENTINEL) {
		s->boot_alloc_bytes += new_bytes;
	} else {
		heap_stats_add(s, new_owner, new_bytes);
	}
}

void z_heap_stats_on_init(struct sys_heap *heap)
{
	struct z_heap *h = heap->heap;
	int i;

	for (i = 0; i < z_heap_stats_count; i++) {
		if (z_heap_stats_heaps[i] == heap) {
			memset(&z_heap_stats_pool[i], 0,
			       sizeof(z_heap_stats_pool[i]));
			h->stats = &z_heap_stats_pool[i];
			return;
		}
	}

	if (z_heap_stats_count < CONFIG_SYS_HEAP_STATS_TRACKED_HEAPS) {
		i = z_heap_stats_count++;
		z_heap_stats_heaps[i] = heap;
		memset(&z_heap_stats_pool[i], 0, sizeof(z_heap_stats_pool[i]));
		h->stats = &z_heap_stats_pool[i];
	} else {
		LOG_WRN("sys_heap: stats pool exhausted, heap %p untracked",
			(void *)h);
		h->stats = NULL;
	}
}

void z_heap_stats_on_alloc(struct sys_heap *heap, chunkid_t c, void *mem)
{
	struct z_heap *h = heap->heap;
	struct k_thread *t;

	if (h->stats == NULL) {
		return;
	}

	t = heap_stats_current_thread();
	chunk_trailer(h, c)->thread = t;
	heap_stats_update(h->stats, NULL, 0, t,
			  chunk_usable_bytes(h, c) - mem_align_gap(h, mem));
}

void z_heap_stats_on_free(struct sys_heap *heap, chunkid_t c, void *mem)
{
	struct z_heap *h = heap->heap;
	struct k_thread *old_owner;
	size_t freed_usable;

	if (h->stats == NULL) {
		return;
	}

	old_owner = chunk_trailer(h, c)->thread;
	freed_usable = chunk_usable_bytes(h, c) - mem_align_gap(h, mem);
	heap_stats_update(h->stats, old_owner, freed_usable, NULL, 0);
}

void z_heap_stats_on_realloc(struct sys_heap *heap, chunkid_t c, void *ptr,
			     size_t old_usable, struct k_thread *old_owner)
{
	struct z_heap *h = heap->heap;
	struct k_thread *new_owner;
	size_t new_usable;

	if (h->stats == NULL) {
		return;
	}

	new_owner = heap_stats_current_thread();
	new_usable = chunk_usable_bytes(h, c) - mem_align_gap(h, ptr);
	chunk_trailer(h, c)->thread = new_owner;
	heap_stats_update(h->stats, old_owner, old_usable, new_owner, new_usable);
}

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

	isr  = h->stats->isr_alloc_bytes;
	boot = h->stats->boot_alloc_bytes;
	ovfl = h->stats->overflow_alloc_bytes;

	if (isr > 0) {
		LOG_INF("  ISR context:         %zu bytes allocated", isr);
	}
	if (boot > 0) {
		LOG_INF("  pre-scheduler boot:  %zu bytes allocated", boot);
	}
	if (ovfl > 0) {
		LOG_INF("  overflow (untracked threads): %zu bytes allocated", ovfl);
	}

	n = h->stats->num_threads;

	for (i = 0; i < n; i++) {
		struct z_heap_thread_stat *t = &h->stats->threads[i];

#if defined(CONFIG_THREAD_NAME)
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

	/*
	 * Reject out-of-range or free chunks; trailers on free chunks
	 * overlap with free-list pointer fields and hold garbage.
	 */
	if (c >= h->end_chunk || !chunk_used(h, c)) {
		return NULL;
	}

	return chunk_trailer(h, c)->caller;
}

#endif /* CONFIG_SYS_HEAP_CALLER_POINTER */
