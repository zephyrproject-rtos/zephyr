/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/cache.h>
#include <zephyr/logging/log.h>
#include <fsl_cache.h>

#if defined(CONFIG_DCACHE)
static inline bool cache_data_is_enabled(void)
{
	return (LMEM->PSCCR & LMEM_PSCCR_ENCACHE_MASK) != 0U;
}

static inline int cache_data_require_enabled(void)
{
	return cache_data_is_enabled() ? 0 : -EAGAIN;
}

static int cache_data_manage_range(void *addr, size_t size,
				   void (*operation)(uint32_t, uint32_t))
{
	int ret = cache_data_require_enabled();

	if (ret != 0) {
		return ret;
	}

	operation((uint32_t)addr, (uint32_t)size);

	return 0;
}

void cache_data_enable(void)
{
	L1CACHE_EnableSystemCache();
}

void cache_data_disable(void)
{
	if (!cache_data_is_enabled()) {
		return;
	}

	L1CACHE_DisableSystemCache();
}

int cache_data_flush_all(void)
{
	int ret = cache_data_require_enabled();

	if (ret != 0) {
		return ret;
	}

	L1CACHE_CleanSystemCache();

	return 0;
}

int cache_data_invd_all(void)
{
	int ret = cache_data_require_enabled();

	if (ret != 0) {
		return ret;
	}

	L1CACHE_InvalidateSystemCache();

	return 0;
}

int cache_data_flush_and_invd_all(void)
{
	int ret = cache_data_require_enabled();

	if (ret != 0) {
		return ret;
	}

	L1CACHE_CleanInvalidateSystemCache();

	return 0;
}

int cache_data_flush_range(void *addr, size_t size)
{
	return cache_data_manage_range(addr, size, L1CACHE_CleanSystemCacheByRange);
}

int cache_data_invd_range(void *addr, size_t size)
{
	return cache_data_manage_range(addr, size,
				       L1CACHE_InvalidateSystemCacheByRange);
}

int cache_data_flush_and_invd_range(void *addr, size_t size)
{
	return cache_data_manage_range(addr, size,
				       L1CACHE_CleanInvalidateSystemCacheByRange);
}

#endif

#if defined(CONFIG_ICACHE)

static inline bool cache_instr_is_enabled(void)
{
	return (LMEM->PCCCR & LMEM_PCCCR_ENCACHE_MASK) != 0U;
}

static inline int cache_instr_require_enabled(void)
{
	return cache_instr_is_enabled() ? 0 : -EAGAIN;
}

static int cache_instr_manage_range(void *addr, size_t size,
				    void (*operation)(uint32_t, uint32_t))
{
	int ret = cache_instr_require_enabled();

	if (ret != 0) {
		return ret;
	}

	operation((uint32_t)addr, (uint32_t)size);

	return 0;
}

void cache_instr_enable(void)
{
	L1CACHE_EnableCodeCache();
}

void cache_instr_disable(void)
{
	if (!cache_instr_is_enabled()) {
		return;
	}

	L1CACHE_DisableCodeCache();
}

int cache_instr_flush_all(void)
{
	int ret = cache_instr_require_enabled();

	if (ret != 0) {
		return ret;
	}

	L1CACHE_CleanCodeCache();

	return 0;
}

int cache_instr_invd_all(void)
{
	int ret = cache_instr_require_enabled();

	if (ret != 0) {
		return ret;
	}

	L1CACHE_InvalidateCodeCache();

	return 0;
}

int cache_instr_flush_and_invd_all(void)
{
	int ret = cache_instr_require_enabled();

	if (ret != 0) {
		return ret;
	}

	L1CACHE_CleanInvalidateCodeCache();

	return 0;
}

int cache_instr_flush_range(void *addr, size_t size)
{
	return cache_instr_manage_range(addr, size,
				       L1CACHE_CleanCodeCacheByRange);
}

int cache_instr_invd_range(void *addr, size_t size)
{
	return cache_instr_manage_range(addr, size,
				       L1CACHE_InvalidateCodeCacheByRange);
}

int cache_instr_flush_and_invd_range(void *addr, size_t size)
{
	return cache_instr_manage_range(addr, size,
				       L1CACHE_CleanInvalidateCodeCacheByRange);
}
#endif
