/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/cache.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#include <fsl_cache.h>

LOG_MODULE_REGISTER(cache_nxp_xcache, CONFIG_CACHE_LOG_LEVEL);

#if !defined(NXP_XCACHE_INSTR)
#define NXP_XCACHE_INSTR XCACHE_PC
#endif

#if !defined(NXP_XCACHE_DATA)
#define NXP_XCACHE_DATA XCACHE_PS
#endif

void cache_data_enable(void)
{
	XCACHE_EnableCache(NXP_XCACHE_DATA);
}

void cache_data_disable(void)
{
	XCACHE_DisableCache(NXP_XCACHE_DATA);
}

int cache_data_flush_all(void)
{
	XCACHE_CleanCache(NXP_XCACHE_DATA);

	return 0;
}

int cache_data_invd_all(void)
{
	/*
	 * A whole-cache invalidate would drop every data-cache line, including
	 * the dirty lines backing the running stack and other live state in
	 * cached SRAM. XCACHE has no "invalidate only" that can spare them, and
	 * flushing right before invalidating does not help: the stack is dirtied
	 * again between the flush and the invalidate, so that live data is still
	 * lost and the system is corrupted. The Arm Cortex-M L1 backend refuses
	 * the same operation for the same reason (arch/arm/core/cortex_m/cache.c).
	 * Report it as unsupported instead of corrupting memory; callers that
	 * must drop a specific buffer use cache_data_invd_range().
	 */
	return -ENOTSUP;
}

int cache_data_flush_and_invd_all(void)
{
	XCACHE_CleanInvalidateCache(NXP_XCACHE_DATA);

	return 0;
}

int cache_data_flush_range(void *addr, size_t size)
{
	XCACHE_CleanCacheByRange((uint32_t)addr, size);

	return 0;
}

int cache_data_invd_range(void *addr, size_t size)
{
	XCACHE_InvalidateCacheByRange((uint32_t)addr, size);

	return 0;
}

int cache_data_flush_and_invd_range(void *addr, size_t size)
{
	XCACHE_CleanInvalidateCacheByRange((uint32_t)addr, size);

	return 0;
}

void cache_instr_enable(void)
{
	XCACHE_EnableCache(NXP_XCACHE_INSTR);
}

void cache_instr_disable(void)
{
	XCACHE_DisableCache(NXP_XCACHE_INSTR);
}

int cache_instr_flush_all(void)
{
	XCACHE_CleanCache(NXP_XCACHE_INSTR);

	return 0;
}

int cache_instr_invd_all(void)
{
	XCACHE_InvalidateCache(NXP_XCACHE_INSTR);

	return 0;
}

int cache_instr_flush_and_invd_all(void)
{
	XCACHE_CleanInvalidateCache(NXP_XCACHE_INSTR);

	return 0;
}

int cache_instr_flush_range(void *addr, size_t size)
{
	XCACHE_CleanCacheByRange((uint32_t)addr, size);

	return 0;
}

int cache_instr_invd_range(void *addr, size_t size)
{
	XCACHE_InvalidateCacheByRange((uint32_t)addr, size);

	return 0;
}

int cache_instr_flush_and_invd_range(void *addr, size_t size)
{
	XCACHE_CleanInvalidateCacheByRange((uint32_t)addr, size);

	return 0;
}
