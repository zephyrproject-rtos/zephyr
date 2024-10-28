/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Cortex-A/R AArch32 L1-cache maintenance operations.
 *
 * This module implement the cache API for Cortex-A/R AArch32 cores using CMSIS.
 * Only L1-cache maintenance operations is supported.
 */

#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <cmsis_core.h>
#include <zephyr/sys/barrier.h>

/* Cache Type Register */
#define	CTR_DMINLINE_SHIFT	16
#define	CTR_DMINLINE_MASK	BIT_MASK(4)

#ifdef CONFIG_DCACHE

static size_t dcache_line_size;

/**
 * @brief Get the smallest D-cache line size.
 *
 * Get the smallest D-cache line size of all the data and unified caches that
 * the processor controls.
 */
size_t arch_dcache_line_size_get(void)
{
	uint32_t val;
	uint32_t dminline;

	if (!dcache_line_size) {
		val = read_sysreg(ctr);
		dminline = (val >> CTR_DMINLINE_SHIFT) & CTR_DMINLINE_MASK;
		/* Log2 of the number of words */
		dcache_line_size = 2 << dminline;
	}

	return dcache_line_size;
}

void arch_dcache_enable(void)
{
	uint32_t val;

	arch_dcache_invd_all();

	val = __get_SCTLR();
	val |= SCTLR_C_Msk;
	barrier_dsync_fence_full();
	__set_SCTLR(val);
	barrier_isync_fence_full();
}

void arch_dcache_disable(void)
{
	uint32_t val;

	val = __get_SCTLR();
	val &= ~SCTLR_C_Msk;
	barrier_dsync_fence_full();
	__set_SCTLR(val);
	barrier_isync_fence_full();

	arch_dcache_flush_and_invd_all();
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
	size_t line_size;
	uintptr_t addr = (uintptr_t)start_addr;
	uintptr_t end_addr = addr + size;

	/* Align address to line size */
	line_size = arch_dcache_line_size_get();
	addr &= ~(line_size - 1);

	while (addr < end_addr) {
		L1C_CleanDCacheMVA((void *)addr);
		addr += line_size;
	}

	return 0;
}

int arch_dcache_invd_range(void *start_addr, size_t size)
{
	size_t line_size;
	uintptr_t addr = (uintptr_t)start_addr;
	uintptr_t end_addr = addr + size;

	line_size = arch_dcache_line_size_get();

	/*
	 * Clean and invalidate the partial cache lines at both ends of the
	 * given range to prevent data corruption
	 */
	if (end_addr & (line_size - 1)) {
		end_addr &= ~(line_size - 1);
		L1C_CleanInvalidateDCacheMVA((void *)end_addr);
	}

	if (addr & (line_size - 1)) {
		addr &= ~(line_size - 1);
		if (addr == end_addr) {
			goto done;
		}
		L1C_CleanInvalidateDCacheMVA((void *)addr);
		addr += line_size;
	}

	/* Align address to line size */
	addr &= ~(line_size - 1);

	while (addr < end_addr) {
		L1C_InvalidateDCacheMVA((void *)addr);
		addr += line_size;
	}

done:
	return 0;
}

int arch_dcache_flush_and_invd_range(void *start_addr, size_t size)
{
	size_t line_size;
	uintptr_t addr = (uintptr_t)start_addr;
	uintptr_t end_addr = addr + size;

	/* Align address to line size */
	line_size = arch_dcache_line_size_get();
	addr &= ~(line_size - 1);

	while (addr < end_addr) {
		L1C_CleanInvalidateDCacheMVA((void *)addr);
		addr += line_size;
	}

	return 0;
}

#endif

#ifdef CONFIG_ICACHE

void arch_icache_enable(void)
{
	arch_icache_invd_all();
	__set_SCTLR(__get_SCTLR() | SCTLR_I_Msk);
	barrier_isync_fence_full();
}

void arch_icache_disable(void)
{
	__set_SCTLR(__get_SCTLR() & ~SCTLR_I_Msk);
	barrier_isync_fence_full();
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

#endif

void arch_cache_init(void)
{
}
