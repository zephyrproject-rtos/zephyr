/*
 * Copyright (c) 2019,2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/sys_heap.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include "heap.h"

int sys_heap_runtime_stats_get(struct sys_heap *heap,
		struct sys_memory_stats *stats)
{
	if ((heap == NULL) || (stats == NULL)) {
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
