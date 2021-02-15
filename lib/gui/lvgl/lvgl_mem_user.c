/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lvgl_mem.h"
#include <zephyr.h>
#include <init.h>
#include <sys/sys_heap.h>


#define HEAP_BYTES (CONFIG_LVGL_MEM_POOL_MAX_SIZE * \
		    CONFIG_LVGL_MEM_POOL_NUMBER_BLOCKS)

static char lvgl_heap_mem[HEAP_BYTES];
static struct sys_heap lvgl_heap;

void *lvgl_malloc(size_t size)
{
	return sys_heap_alloc(&lvgl_heap, size);
}

void lvgl_free(void *ptr)
{
	sys_heap_free(&lvgl_heap, ptr);
}

static int lvgl_heap_init(const struct device *unused)
{
	sys_heap_init(&lvgl_heap, &lvgl_heap_mem[0], HEAP_BYTES);
	return 0;
}

SYS_INIT(lvgl_heap_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
