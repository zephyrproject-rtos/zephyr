/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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

static struct k_spinlock xcache_lock;

void cache_data_enable(void)
{
	k_spinlock_key_t key = k_spin_lock(&xcache_lock);

	XCACHE_EnableCache(NXP_XCACHE_DATA);
	k_spin_unlock(&xcache_lock, key);
}

void cache_data_disable(void)
{
	k_spinlock_key_t key = k_spin_lock(&xcache_lock);

	XCACHE_DisableCache(NXP_XCACHE_DATA);
	k_spin_unlock(&xcache_lock, key);
}

int cache_data_flush_all(void)
{
	k_spinlock_key_t key = k_spin_lock(&xcache_lock);

	XCACHE_CleanCache(NXP_XCACHE_DATA);
	k_spin_unlock(&xcache_lock, key);

	return 0;
}

int cache_data_invd_all(void)
{
	k_spinlock_key_t key = k_spin_lock(&xcache_lock);

	XCACHE_InvalidateCache(NXP_XCACHE_DATA);
	k_spin_unlock(&xcache_lock, key);

	return 0;
}

int cache_data_flush_and_invd_all(void)
{
	k_spinlock_key_t key = k_spin_lock(&xcache_lock);

	XCACHE_CleanInvalidateCache(NXP_XCACHE_DATA);
	k_spin_unlock(&xcache_lock, key);

	return 0;
}

int cache_data_flush_range(void *addr, size_t size)
{
	k_spinlock_key_t key = k_spin_lock(&xcache_lock);

	XCACHE_CleanCacheByRange((uint32_t)addr, size);
	k_spin_unlock(&xcache_lock, key);

	return 0;
}

int cache_data_invd_range(void *addr, size_t size)
{
	k_spinlock_key_t key = k_spin_lock(&xcache_lock);

	XCACHE_InvalidateCacheByRange((uint32_t)addr, size);
	k_spin_unlock(&xcache_lock, key);

	return 0;
}

int cache_data_flush_and_invd_range(void *addr, size_t size)
{
	k_spinlock_key_t key = k_spin_lock(&xcache_lock);

	XCACHE_CleanInvalidateCacheByRange((uint32_t)addr, size);
	k_spin_unlock(&xcache_lock, key);

	return 0;
}

void cache_instr_enable(void)
{
	k_spinlock_key_t key = k_spin_lock(&xcache_lock);

	XCACHE_EnableCache(NXP_XCACHE_INSTR);
	k_spin_unlock(&xcache_lock, key);
}

void cache_instr_disable(void)
{
	k_spinlock_key_t key = k_spin_lock(&xcache_lock);

	XCACHE_DisableCache(NXP_XCACHE_INSTR);
	k_spin_unlock(&xcache_lock, key);
}

int cache_instr_flush_all(void)
{
	k_spinlock_key_t key = k_spin_lock(&xcache_lock);

	XCACHE_CleanCache(NXP_XCACHE_INSTR);
	k_spin_unlock(&xcache_lock, key);

	return 0;
}

int cache_instr_invd_all(void)
{
	k_spinlock_key_t key = k_spin_lock(&xcache_lock);

	XCACHE_InvalidateCache(NXP_XCACHE_INSTR);
	k_spin_unlock(&xcache_lock, key);

	return 0;
}

int cache_instr_flush_and_invd_all(void)
{
	k_spinlock_key_t key = k_spin_lock(&xcache_lock);

	XCACHE_CleanInvalidateCache(NXP_XCACHE_INSTR);
	k_spin_unlock(&xcache_lock, key);

	return 0;
}

int cache_instr_flush_range(void *addr, size_t size)
{
	k_spinlock_key_t key = k_spin_lock(&xcache_lock);

	XCACHE_CleanCacheByRange((uint32_t)addr, size);
	k_spin_unlock(&xcache_lock, key);

	return 0;
}

int cache_instr_invd_range(void *addr, size_t size)
{
	k_spinlock_key_t key = k_spin_lock(&xcache_lock);

	XCACHE_InvalidateCacheByRange((uint32_t)addr, size);
	k_spin_unlock(&xcache_lock, key);

	return 0;
}

int cache_instr_flush_and_invd_range(void *addr, size_t size)
{
	k_spinlock_key_t key = k_spin_lock(&xcache_lock);

	XCACHE_CleanInvalidateCacheByRange((uint32_t)addr, size);
	k_spin_unlock(&xcache_lock, key);

	return 0;
}
