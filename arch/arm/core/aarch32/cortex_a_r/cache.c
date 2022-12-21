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
#include <zephyr/arch/arm/aarch32/cortex_a_r/cmsis.h>

void arch_dcache_enable(void)
{
	L1C_EnableCaches();
}

void arch_dcache_disable(void)
{
	L1C_DisableCaches();
}

int arch_dcache_flush_all(void)
{
	L1C_CleanDCacheAll();

	return 0;
}

int arch_dcache_invd_all(void)
{
	L1C_InvalidateDCacheAll();

	return 0;
}

int arch_dcache_flush_and_invd_all(void)
{
	L1C_CleanInvalidateDCacheAll();

	return 0;
}

int arch_dcache_flush_range(void *start_addr, size_t size)
{
	uintptr_t addr = (uintptr_t)start_addr;
	uintptr_t end_addr = (uintptr_t)addr + size;

	addr /= CONFIG_DCACHE_LINE_SIZE;
	addr *= CONFIG_DCACHE_LINE_SIZE;

	for (; addr < end_addr; addr += CONFIG_DCACHE_LINE_SIZE)
		L1C_CleanDCacheMVA((void *)addr);

	return 0;
}

int arch_dcache_invd_range(void *start_addr, size_t size)
{
	uintptr_t addr = (uintptr_t)start_addr;
	uintptr_t end_addr = (uintptr_t)addr + size;

	addr /= CONFIG_DCACHE_LINE_SIZE;
	addr *= CONFIG_DCACHE_LINE_SIZE;

	for (; addr < end_addr; addr += CONFIG_DCACHE_LINE_SIZE)
		L1C_InvalidateDCacheMVA((void *)start_addr);

	return 0;
}

int arch_dcache_flush_and_invd_range(void *start_addr, size_t size)
{
	uintptr_t addr = (uintptr_t)start_addr;
	uintptr_t end_addr = (uintptr_t)addr + size;

	addr /= CONFIG_DCACHE_LINE_SIZE;
	addr *= CONFIG_DCACHE_LINE_SIZE;

	for (; addr < end_addr; addr += CONFIG_DCACHE_LINE_SIZE)
		L1C_CleanInvalidateDCacheMVA((void *)start_addr);

	return 0;
}

void arch_icache_enable(void)
{
	L1C_EnableCaches();
}

void arch_icache_disable(void)
{
	L1C_DisableCaches();
}

int arch_icache_flush_all(void)
{
	return -ENOTSUP;
}

int arch_icache_invd_all(void)
{
	L1C_InvalidateICacheAll();

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
	return -ENOTSUP;
}

int arch_icache_flush_and_invd_range(void *start_addr, size_t size)
{
	return -ENOTSUP;
}
