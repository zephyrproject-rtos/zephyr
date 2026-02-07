/*
 * Copyright (c) 2026 Qualcomm Technologies, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Custom heap header implementation
 *
 * This module implements custom per-chunk headers for Zephyr's sys_heap,
 * enabling per-thread allocation statistics tracking. The implementation
 * uses thread-local storage to maintain per-thread identifiers and tracks
 * allocation sizes for each thread across multiple heaps.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#include <stdint.h>
#include <string.h>

#include "heap_custom_header.h"

__thread uint32_t z_thread_identifier = UINT32_MAX;
static atomic_t z_global_tid_cnt = ATOMIC_INIT(0);

/* Buffer holding per-thread statistics names */
static struct z_heap_stats_thread_info
z_heap_stats_thread_info_buffer[CONFIG_SYS_HEAP_STATS_MAX_THREADS] = { 0 };

void z_heap_set_custom_header(struct z_heap *h, chunkid_t chunk_id,
				   const struct z_heap_custom_header *custom_hdr)
{
	__ASSERT(h != NULL, "heap pointer is NULL");
	__ASSERT(custom_hdr != NULL, "custom header pointer is NULL");
	__ASSERT(chunk_id < h->end_chunk, "chunk_id out of range");

	struct z_heap_custom_header *hdr_ptr =
		(struct z_heap_custom_header *)chunk_buf(h)[chunk_id].bytes;

	hdr_ptr->global_tid = custom_hdr->global_tid;
	hdr_ptr->caller_address = custom_hdr->caller_address;
}

uint32_t z_heap_get_tid_and_update_stats_add(struct sys_heap *heap,
					     chunksz_t actual_allocated_chunk_sz)
{
	if (heap == NULL) {
		return UINT32_MAX;
	}

	struct z_heap *h = heap->heap;

	if (h == NULL) {
		return UINT32_MAX;
	}

	if (h->heap_stats_buffer_ptr == NULL) {
		return UINT32_MAX;
	}

	/* Assign thread ID on first allocation */
	if (z_thread_identifier == UINT32_MAX) {
		const int MAX_RETRIES = 10;

		for (int retry_count = 0; retry_count < MAX_RETRIES; retry_count++) {
			uint32_t new_id = atomic_get(&z_global_tid_cnt);

			if (new_id >= CONFIG_SYS_HEAP_STATS_MAX_THREADS) {
				z_thread_identifier = UINT32_MAX;
				return UINT32_MAX;
			}

			if (atomic_cas(&z_global_tid_cnt, new_id, new_id + 1)) {
				z_thread_identifier = new_id;

				struct z_heap_stats_thread_info *thread_stats =
					&z_heap_stats_thread_info_buffer[z_thread_identifier];

				k_tid_t current_k_tid = k_current_get();
				const char *thread_name = k_thread_name_get(current_k_tid);

				if (thread_name == NULL) {
					thread_name = "<unknown>";
				}

				/* Ensure NULL termination */
				strncpy(thread_stats->thread_name, thread_name,
					sizeof(thread_stats->thread_name) - 1);
				thread_stats->thread_name[
					sizeof(thread_stats->thread_name) - 1] = '\0';

				((struct z_heap_stats *)h->heap_stats_buffer_ptr)
					[z_thread_identifier].alloc_size =
					actual_allocated_chunk_sz * CHUNK_UNIT;

				break;
			}
		}

		if (z_thread_identifier == UINT32_MAX) {
			/* Failed to allocate thread ID */
			return UINT32_MAX;
		}

		return z_thread_identifier;
	}

	/* Update existing thread statistics */
	((struct z_heap_stats *)h->heap_stats_buffer_ptr)
		[z_thread_identifier].alloc_size +=
		actual_allocated_chunk_sz * CHUNK_UNIT;

	return z_thread_identifier;
}

void z_heap_update_stats_sub(struct sys_heap *heap, chunkid_t c)
{
	if (heap == NULL) {
		return;
	}

	struct z_heap *h = heap->heap;

	if (h == NULL || c >= h->end_chunk) {
		return;
	}

	struct z_heap_custom_header *custom_hdr_ptr =
		(struct z_heap_custom_header *)chunk_buf(h)[c].bytes;

	uint32_t allocating_thread_id = custom_hdr_ptr->global_tid;

	if (h->heap_stats_buffer_ptr != NULL &&
	    allocating_thread_id != UINT32_MAX &&
		allocating_thread_id < CONFIG_SYS_HEAP_STATS_MAX_THREADS) {
		struct z_heap_stats *stats =
			&((struct z_heap_stats *)h->heap_stats_buffer_ptr)
			[allocating_thread_id];
		size_t decrement = chunk_size(h, c) * CHUNK_UNIT;

		if (stats->alloc_size >= decrement) {
			stats->alloc_size -= decrement;
		} else {
			stats->alloc_size = 0U;
		}
	}
}
