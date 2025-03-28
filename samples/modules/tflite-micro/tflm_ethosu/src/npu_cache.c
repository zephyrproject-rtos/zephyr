/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stddef.h>
#include <soc.h>
#include <zephyr/cache.h>

/*******************************************************************************
 * Re-implementation/Overrides __((weak)) symbol functions from ethosu_driver.c
 *******************************************************************************/

#ifndef CONFIG_NOCACHE_MEMORY

void ethosu_flush_dcache(uint32_t *p, size_t bytes)
{
	if (p && bytes) {
		sys_cache_data_flush_range((void *)p, bytes);
	}
}

void ethosu_invalidate_dcache(uint32_t *p, size_t bytes)
{
	if (p && bytes) {
		sys_cache_data_invd_range((void *)p, bytes);
	}
}

#endif
