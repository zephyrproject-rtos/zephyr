/*
 * Copyright (c) 2025 Silicon Laboratories Inc. www.silabs.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/sys_heap.h>
#include <zephyr/kernel.h>

static size_t i;
static struct sys_heap *heaps[CONFIG_SYS_HEAP_ARRAY_SIZE];

int sys_heap_array_save(struct sys_heap *heap)
{
	if (heap == NULL) {
		return -EINVAL;
	}

	if (i < CONFIG_SYS_HEAP_ARRAY_SIZE) {
		heaps[i++] = heap;
	} else {
		return -EINVAL;
	}

	return 0;
}

int sys_heap_array_get(struct sys_heap ***heap)
{
	if (heap == NULL) {
		return -EINVAL;
	}

	*heap = heaps;

	return i;
}
