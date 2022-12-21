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

int arch_dcache_flush_all(void)
{
	SCB_CleanDCache();

	return 0;
}

int arch_dcache_invd_all(void)
{
	SCB_InvalidateDCache();

	return 0;
}

int arch_dcache_flush_and_invd_all(void)
{
	SCB_CleanInvalidateDCache();

	return 0;
}

int arch_dcache_flush_range(void *start_addr, size_t size)
{
	SCB_CleanDCache_by_Addr(start_addr, size);

	return 0;
}

int arch_dcache_invd_range(void *start_addr, size_t size)
{
	SCB_InvalidateDCache_by_Addr(start_addr, size);

	return 0;
}

int arch_dcache_flush_and_invd_range(void *start_addr, size_t size)
{
	SCB_CleanInvalidateDCache_by_Addr(start_addr, size);

	return 0;
}

void arch_icache_enable(void)
{
	SCB_EnableICache();
}

void arch_icache_disable(void)
{
	SCB_DisableICache();
}

int arch_icache_flush_all(void)
{
	return -ENOTSUP;
}

int arch_icache_invd_all(void)
{
	SCB_InvalidateICache();

	return 0;
}

int arch_icache_flush_and_invd_all(void)
{
	return -ENOTSUP;
}

int arch_icache_flush_range(void *start_addr, size_t size)
{
	return -ENOTSUP;
}

int arch_icache_invd_range(void *start_addr, size_t size)
{
	SCB_InvalidateICache_by_Addr(start_addr, size);

	return 0;
}

int arch_icache_flush_and_invd_range(void *start_addr, size_t size)
{
	return -ENOTSUP;
}
