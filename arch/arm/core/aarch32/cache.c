/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2020-2022 Qualcomm Innovation Center, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file
 * @brief Cache manipulation
 *
 * This module contains functions for manipulation caches.
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/cache.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>

void arch_dcache_enable(void)
{
	SCB_EnableDCache();
}

void arch_dcache_disable(void)
{
	SCB_DisableDCache();
}

void arch_icache_enable(void)
{
	SCB_EnableICache();
}

void arch_icache_disable(void)
{
	SCB_DisableICache();
}

static void arch_dcache_flush(void *start_addr, size_t size)
{
	SCB_CleanDCache_by_Addr(start_addr, size);
}

static void arch_dcache_invd(void *start_addr, size_t size)
{
	SCB_InvalidateDCache_by_Addr(start_addr, size);
}

static void arch_dcache_flush_and_invd(void *start_addr, size_t size)
{
	SCB_CleanInvalidateDCache_by_Addr(start_addr, size);
}

static void arch_icache_invd(void *start_addr, size_t size)
{
	SCB_InvalidateICache_by_Addr(start_addr, size);
}

int arch_dcache_range(void *addr, size_t size, int op)
{
	if (op == K_CACHE_INVD) {
		arch_dcache_invd(addr, size);
	} else if (op == K_CACHE_WB) {
		arch_dcache_flush(addr, size);
	} else if (op == K_CACHE_WB_INVD) {
		arch_dcache_flush_and_invd(addr, size);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

int arch_dcache_all(int op)
{
	if (op == K_CACHE_INVD) {
		SCB_InvalidateDCache();
	} else if (op == K_CACHE_WB) {
		SCB_CleanDCache();
	} else if (op == K_CACHE_WB_INVD) {
		SCB_CleanInvalidateDCache();
	} else {
		return -ENOTSUP;
	}

	return 0;
}

int arch_icache_all(int op)
{
	if (op == K_CACHE_INVD) {
		SCB_InvalidateICache();
	} else {
		return -ENOTSUP;
	}

	return 0;
}

int arch_icache_range(void *addr, size_t size, int op)
{
	if (op == K_CACHE_INVD) {
		arch_icache_invd(addr, size);
	} else {
		return -ENOTSUP;
	}

	return 0;
}
