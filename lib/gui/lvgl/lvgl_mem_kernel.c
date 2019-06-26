/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lvgl_mem.h"
#include <zephyr.h>
#include <init.h>
#include <sys/mempool.h>

K_MEM_POOL_DEFINE(lvgl_mem_pool,
		CONFIG_LVGL_MEM_POOL_MIN_SIZE,
		CONFIG_LVGL_MEM_POOL_MAX_SIZE,
		CONFIG_LVGL_MEM_POOL_NUMBER_BLOCKS, 4);

void *lvgl_malloc(size_t size)
{
	return k_mem_pool_malloc(&lvgl_mem_pool, size);
}

void lvgl_free(void *ptr)
{
	k_free(ptr);
}
