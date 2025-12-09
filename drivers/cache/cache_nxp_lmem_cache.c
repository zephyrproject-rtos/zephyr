/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/cache.h>
#include <zephyr/logging/log.h>
#include <fsl_cache.h>

void cache_instr_enable(void)
{
	L1CACHE_EnableCodeCache();
}

void cache_instr_disable(void)
{
	L1CACHE_DisableCodeCache();
}

int cache_instr_flush_all(void)
{
	L1CACHE_CleanCodeCache();

	return 0;
}

int cache_instr_invd_all(void)
{
	L1CACHE_InvalidateCodeCache();

	return 0;
}

int cache_instr_flush_and_invd_all(void)
{
	L1CACHE_CleanInvalidateCodeCache();

	return 0;
}

int cache_instr_flush_range(void *addr, size_t size)
{
	L1CACHE_CleanCodeCacheByRange((uint32_t)addr, (uint32_t)size);

	return 0;
}

int cache_instr_invd_range(void *addr, size_t size)
{
	L1CACHE_InvalidateCodeCacheByRange((uint32_t)addr, (uint32_t)size);

	return 0;
}

int cache_instr_flush_and_invd_range(void *addr, size_t size)
{
	L1CACHE_CleanInvalidateCodeCacheByRange((uint32_t)addr, (uint32_t)size);

	return 0;
}
