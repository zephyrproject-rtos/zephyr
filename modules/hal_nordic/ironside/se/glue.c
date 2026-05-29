/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ironside/se/glue.h>
#include <zephyr/cache.h>

void ironside_se_data_cache_writeback(void *addr, size_t size)
{
	sys_cache_data_flush_range(addr, size);
}

void ironside_se_data_cache_invalidate(void *addr, size_t size)
{
	sys_cache_data_invd_range(addr, size);
}

void ironside_se_data_cache_writeback_invalidate(void *addr, size_t size)
{
	sys_cache_data_flush_and_invd_range(addr, size);
}
