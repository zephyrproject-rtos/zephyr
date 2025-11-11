/*
 * Copyright (c) 2025 Henrik Lindblom <henrik.lindblom@vaisala.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/drivers/cache.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/math_extras.h>
#include <stm32_ll_dcache.h>
#include <stm32_ll_icache.h>

LOG_MODULE_REGISTER(cache_stm32, CONFIG_CACHE_LOG_LEVEL);

#ifdef CONFIG_DCACHE

void cache_data_enable(void)
{
	LL_DCACHE_Enable(DCACHE1);
#if defined(DCACHE2)
	LL_DCACHE_Enable(DCACHE2);
#endif
}

void cache_data_disable(void)
{
	cache_data_flush_all();

	while (LL_DCACHE_IsActiveFlag_BUSYCMD(DCACHE1)) {
	}

	LL_DCACHE_Disable(DCACHE1);
	LL_DCACHE_ClearFlag_BSYEND(DCACHE1);

#if defined(DCACHE2)
	while (LL_DCACHE_IsActiveFlag_BUSYCMD(DCACHE2)) {
	}

	LL_DCACHE_Disable(DCACHE2);
	LL_DCACHE_ClearFlag_BSYEND(DCACHE2);
#endif
}

static int cache_data_manage_range(void *addr, size_t size, uint32_t command)
{
	/*
	 * This is a simple approach to invalidate the range. The address might be in either DCACHE1
	 * or DCACHE2 (if present). The cache invalidation algorithm checks the TAG memory for the
	 * specified address range so there's little harm in just checking both caches.
	 */
	uint32_t start = (uint32_t)addr;
	uint32_t end;

	if (u32_add_overflow(start, size, &end)) {
		return -EOVERFLOW;
	}

	LL_DCACHE_SetStartAddress(DCACHE1, start);
	LL_DCACHE_SetEndAddress(DCACHE1, end);
	LL_DCACHE_SetCommand(DCACHE1, command);
	LL_DCACHE_StartCommand(DCACHE1);
#if defined(DCACHE2)
	LL_DCACHE_SetStartAddress(DCACHE2, start);
	LL_DCACHE_SetEndAddress(DCACHE2, end);
	LL_DCACHE_SetCommand(DCACHE2, command);
	LL_DCACHE_StartCommand(DCACHE2);
#endif
	return 0;
}

int cache_data_flush_range(void *addr, size_t size)
{
	return cache_data_manage_range(addr, size, LL_DCACHE_COMMAND_CLEAN_BY_ADDR);
}

int cache_data_invd_range(void *addr, size_t size)
{
	return cache_data_manage_range(addr, size, LL_DCACHE_COMMAND_INVALIDATE_BY_ADDR);
}

int cache_data_flush_and_invd_range(void *addr, size_t size)
{
	return cache_data_manage_range(addr, size, LL_DCACHE_COMMAND_CLEAN_INVALIDATE_BY_ADDR);
}

int cache_data_flush_all(void)
{
	return cache_data_flush_range(0, UINT32_MAX);
}

int cache_data_invd_all(void)
{
	LL_DCACHE_Invalidate(DCACHE1);
#if defined(DCACHE2)
	LL_DCACHE_Invalidate(DCACHE2);
#endif
	return 0;
}

int cache_data_flush_and_invd_all(void)
{
	return cache_data_flush_and_invd_range(0, UINT32_MAX);
}

#endif /* CONFIG_DCACHE */

static inline void wait_for_icache(void)
{
	while (LL_ICACHE_IsActiveFlag_BUSY()) {
	}

	/* Clear BSYEND to avoid an extra interrupt if somebody enables them. */
	LL_ICACHE_ClearFlag_BSYEND();
}

void cache_instr_enable(void)
{
	if (IS_ENABLED(CONFIG_CACHE_STM32_ICACHE_DIRECT_MAPPING)) {
		LL_ICACHE_SetMode(LL_ICACHE_1WAY);
	}

	/*
	 * Need to wait until any pending cache invalidation operations finish. This is recommended
	 * in the reference manual to ensure execution timing determinism.
	 */
	wait_for_icache();
	LL_ICACHE_Enable();
}

void cache_instr_disable(void)
{
	LL_ICACHE_Disable();

	while (LL_ICACHE_IsEnabled()) {
		/**
		 * Wait until the ICACHE is disabled (CR.EN=0), at which point
		 * all requests bypass the cache and are forwarded directly
		 * from the ICACHE slave port to the ICACHE master port(s).
		 *
		 * The cache invalidation will start once disabled, but we allow
		 * it to proceed in the background since it doesn't need to be
		 * complete for requests to bypass the ICACHE.
		 */
	}
}

int cache_instr_flush_all(void)
{
	return -ENOTSUP;
}

int cache_instr_invd_all(void)
{
	LL_ICACHE_Invalidate();
	return 0;
}

int cache_instr_flush_and_invd_all(void)
{
	return -ENOTSUP;
}

int cache_instr_flush_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);
	return -ENOTSUP;
}

int cache_instr_invd_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);
	return -ENOTSUP;
}

int cache_instr_flush_and_invd_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);
	return -ENOTSUP;
}
