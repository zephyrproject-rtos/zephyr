/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/cache.h>
#include <hal/nrf_cache.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cache_nrfx, CONFIG_CACHE_LOG_LEVEL);

#if !defined(NRF_ICACHE) && defined(NRF_CACHE)
#define NRF_ICACHE NRF_CACHE
#endif

#define CACHE_LINE_SIZE		     32
#define CACHE_BUSY_RETRY_INTERVAL_US 10

static struct k_spinlock lock;

enum k_nrf_cache_op {
	/*
	 * Sequentially loop through all dirty lines and write those data units to
	 * memory.
	 *
	 * This is FLUSH in Zephyr nomenclature.
	 */
	K_NRF_CACHE_CLEAN,

	/*
	 * Mark all lines as invalid, ignoring any dirty data.
	 *
	 * This is INVALIDATE in Zephyr nomenclature.
	 */
	K_NRF_CACHE_INVD,

	/*
	 * Clean followed by invalidate
	 *
	 * This is FLUSH_AND_INVALIDATE in Zephyr nomenclature.
	 */
	K_NRF_CACHE_FLUSH,
};

static inline bool is_cache_busy(NRF_CACHE_Type *cache)
{
#if NRF_CACHE_HAS_STATUS
	return nrf_cache_busy_check(cache);
#else
	return false;
#endif
}

static inline void wait_for_cache(NRF_CACHE_Type *cache)
{
	while (is_cache_busy(cache)) {
		k_busy_wait(CACHE_BUSY_RETRY_INTERVAL_US);
	}
}

static inline int _cache_all(NRF_CACHE_Type *cache, enum k_nrf_cache_op op)
{
	/*
	 * We really do not want to invalidate the whole cache.
	 */
	if (op == K_NRF_CACHE_INVD) {
		return -ENOTSUP;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);

	/*
	 * Invalidating the whole cache is dangerous. For good measure
	 * disable the cache.
	 */
	nrf_cache_disable(cache);

	wait_for_cache(cache);

	switch (op) {

#if NRF_CACHE_HAS_TASK_CLEAN
	case K_NRF_CACHE_CLEAN:
		nrf_cache_task_trigger(cache, NRF_CACHE_TASK_CLEANCACHE);
		break;
#endif

	case K_NRF_CACHE_INVD:
		nrf_cache_task_trigger(cache, NRF_CACHE_TASK_INVALIDATECACHE);
		break;

#if NRF_CACHE_HAS_TASK_FLUSH
	case K_NRF_CACHE_FLUSH:
		nrf_cache_task_trigger(cache, NRF_CACHE_TASK_FLUSHCACHE);
		break;
#endif

	default:
		break;
	}

	wait_for_cache(cache);

	nrf_cache_enable(cache);

	k_spin_unlock(&lock, key);

	return 0;
}

static inline void _cache_line(NRF_CACHE_Type *cache, enum k_nrf_cache_op op, uintptr_t line_addr)
{
	wait_for_cache(cache);

	nrf_cache_lineaddr_set(cache, line_addr);

	switch (op) {

#if NRF_CACHE_HAS_TASK_CLEAN
	case K_NRF_CACHE_CLEAN:
		nrf_cache_task_trigger(cache, NRF_CACHE_TASK_CLEANLINE);
		break;
#endif

	case K_NRF_CACHE_INVD:
		nrf_cache_task_trigger(cache, NRF_CACHE_TASK_INVALIDATELINE);
		break;

#if NRF_CACHE_HAS_TASK_FLUSH
	case K_NRF_CACHE_FLUSH:
		nrf_cache_task_trigger(cache, NRF_CACHE_TASK_FLUSHLINE);
		break;
#endif

	default:
		break;
	}

	wait_for_cache(cache);
}

static inline int _cache_range(NRF_CACHE_Type *cache, enum k_nrf_cache_op op, void *addr,
			       size_t size)
{
	uintptr_t line_addr = (uintptr_t)addr;
	uintptr_t end_addr = line_addr + size;

	/*
	 * Align address to line size
	 */
	line_addr &= ~(CACHE_LINE_SIZE - 1);

	do {
		k_spinlock_key_t key = k_spin_lock(&lock);

		_cache_line(cache, op, line_addr);

		k_spin_unlock(&lock, key);

		line_addr += CACHE_LINE_SIZE;

	} while (line_addr < end_addr);

	return 0;
}

static inline int _cache_checks(NRF_CACHE_Type *cache, enum k_nrf_cache_op op, void *addr,
				size_t size, bool is_range)
{
	/* Check if the cache is enabled */
	if (!(cache->ENABLE & CACHE_ENABLE_ENABLE_Enabled)) {
		return -EAGAIN;
	}

	if (!is_range) {
		return _cache_all(cache, op);
	}

	/* Check for invalid address or size */
	if ((!addr) || (!size)) {
		return -EINVAL;
	}

	return _cache_range(cache, op, addr, size);
}

#if defined(NRF_DCACHE) && NRF_CACHE_HAS_TASKS

void cache_data_enable(void)
{
	nrf_cache_enable(NRF_DCACHE);
}

void cache_data_disable(void)
{
	nrf_cache_disable(NRF_DCACHE);
}

int cache_data_flush_all(void)
{
#if NRF_CACHE_HAS_TASK_CLEAN
	return _cache_checks(NRF_DCACHE, K_NRF_CACHE_CLEAN, NULL, 0, false);
#else
	return -ENOTSUP;
#endif
}

int cache_data_invd_all(void)
{
	return _cache_checks(NRF_DCACHE, K_NRF_CACHE_INVD, NULL, 0, false);
}

int cache_data_flush_and_invd_all(void)
{
#if NRF_CACHE_HAS_TASK_FLUSH
	return _cache_checks(NRF_DCACHE, K_NRF_CACHE_FLUSH, NULL, 0, false);
#else
	return -ENOTSUP;
#endif
}

int cache_data_flush_range(void *addr, size_t size)
{
#if NRF_CACHE_HAS_TASK_CLEAN
	return _cache_checks(NRF_DCACHE, K_NRF_CACHE_CLEAN, addr, size, true);
#else
	return -ENOTSUP;
#endif
}

int cache_data_invd_range(void *addr, size_t size)
{
	return _cache_checks(NRF_DCACHE, K_NRF_CACHE_INVD, addr, size, true);
}

int cache_data_flush_and_invd_range(void *addr, size_t size)
{
#if NRF_CACHE_HAS_TASK_FLUSH
	return _cache_checks(NRF_DCACHE, K_NRF_CACHE_FLUSH, addr, size, true);
#else
	return -ENOTSUP;
#endif
}

#else

void cache_data_enable(void)
{
	/* Nothing */
}

void cache_data_disable(void)
{
	/* Nothing */
}

int cache_data_flush_all(void)
{
	return -ENOTSUP;
}

int cache_data_invd_all(void)
{
	return -ENOTSUP;
}

int cache_data_flush_and_invd_all(void)
{
	return -ENOTSUP;
}

int cache_data_flush_range(void *addr, size_t size)
{
	return -ENOTSUP;
}

int cache_data_invd_range(void *addr, size_t size)
{
	return -ENOTSUP;
}

int cache_data_flush_and_invd_range(void *addr, size_t size)
{
	return -ENOTSUP;
}

#endif /* NRF_DCACHE */

#if defined(NRF_ICACHE) && NRF_CACHE_HAS_TASKS

void cache_instr_enable(void)
{
	nrf_cache_enable(NRF_ICACHE);
}

void cache_instr_disable(void)
{
	nrf_cache_disable(NRF_ICACHE);
}

int cache_instr_flush_all(void)
{
#if NRF_CACHE_HAS_TASK_CLEAN
	return _cache_checks(NRF_ICACHE, K_NRF_CACHE_CLEAN, NULL, 0, false);
#else
	return -ENOTSUP;
#endif
}

int cache_instr_invd_all(void)
{
	return _cache_checks(NRF_ICACHE, K_NRF_CACHE_INVD, NULL, 0, false);
}

int cache_instr_flush_and_invd_all(void)
{
#if NRF_CACHE_HAS_TASK_FLUSH
	return _cache_checks(NRF_ICACHE, K_NRF_CACHE_FLUSH, NULL, 0, false);
#else
	return -ENOTSUP;
#endif
}

int cache_instr_flush_range(void *addr, size_t size)
{
#if NRF_CACHE_HAS_TASK_CLEAN
	return _cache_checks(NRF_ICACHE, K_NRF_CACHE_CLEAN, addr, size, true);
#else
	return -ENOTSUP;
#endif
}

int cache_instr_invd_range(void *addr, size_t size)
{
	return _cache_checks(NRF_ICACHE, K_NRF_CACHE_INVD, addr, size, true);
}

int cache_instr_flush_and_invd_range(void *addr, size_t size)
{
#if NRF_CACHE_HAS_TASK_FLUSH
	return _cache_checks(NRF_ICACHE, K_NRF_CACHE_FLUSH, addr, size, true);
#else
	return -ENOTSUP;
#endif
}

#else

void cache_instr_enable(void)
{
	/* Nothing */
}

void cache_instr_disable(void)
{
	/* Nothing */
}

int cache_instr_flush_all(void)
{
	return -ENOTSUP;
}

int cache_instr_invd_all(void)
{
	return -ENOTSUP;
}

int cache_instr_flush_and_invd_all(void)
{
	return -ENOTSUP;
}

int cache_instr_flush_range(void *addr, size_t size)
{
	return -ENOTSUP;
}

int cache_instr_invd_range(void *addr, size_t size)
{
	return -ENOTSUP;
}

int cache_instr_flush_and_invd_range(void *addr, size_t size)
{
	return -ENOTSUP;
}

#endif /* NRF_ICACHE */
