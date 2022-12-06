/*
 * Copyright (c) 2022 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>

#include <string.h>

#include "ext2.h"

/* Initialize heap memory allocator */
K_HEAP_DEFINE(__ext2_heap, CONFIG_EXT2_HEAP_SIZE);
struct k_heap *ext2_heap = &__ext2_heap;

void *ext2_heap_alloc(size_t size)
{
	void *ptr = k_heap_alloc(ext2_heap, size, K_NO_WAIT);

	if (!ptr) {
		return NULL;
	}

	memset(ptr, 0, size);
	return ptr;
}

void  ext2_heap_free(void *ptr)
{
	k_heap_free(ext2_heap, ptr);
}
