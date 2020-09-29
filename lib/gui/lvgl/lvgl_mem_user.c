/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lvgl_mem.h"
#include <zephyr.h>
#include <init.h>
#include <sys/mempool.h>

SYS_MEM_POOL_DEFINE(lvgl_mem_pool, NULL,
		CONFIG_LVGL_MEM_POOL_MIN_SIZE,
		CONFIG_LVGL_MEM_POOL_MAX_SIZE,
		CONFIG_LVGL_MEM_POOL_NUMBER_BLOCKS, 4, .data);

void *lvgl_malloc(size_t size)
{
	return sys_mem_pool_alloc(&lvgl_mem_pool, size);
}

void lvgl_free(void *ptr)
{
	sys_mem_pool_free(ptr);
}

static int lvgl_mem_pool_init(const struct device *unused)
{
	sys_mem_pool_init(&lvgl_mem_pool);
	return 0;
}

SYS_INIT(lvgl_mem_pool_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
