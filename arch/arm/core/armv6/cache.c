/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ARM1176 (ARMv6) L1 cache maintenance via CP15. Replaces the
 * cortex_a_r-derived cache.c that used CMSIS-Core(A) L1C_* helpers --
 * ARM1176 is not a CMSIS target, so those symbols are unavailable and
 * the CP15 c7 register bank is driven directly. Cache line size is
 * fixed at 32 bytes on ARM1176JZF-S.
 *
 * References: ARM ARM (DDI 0100I) chapter B6, ARM1176JZF-S TRM
 * (DDI 0301H) chapter 6.
 */

#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/sys/barrier.h>
#include "../mmu/armv6_cp15.h"

#define SCTLR_C_Msk	BIT(2)
#define SCTLR_I_Msk	BIT(12)

#define ARM1176_CACHE_LINE_BYTES	32

static inline void cp15_dcache_clean_all(void)
{
	uint32_t zero = 0;

	__asm__ volatile("mcr p15, 0, %0, c7, c10, 0" : : "r"(zero) : "memory");
}

static inline void cp15_dcache_invd_all(void)
{
	uint32_t zero = 0;

	__asm__ volatile("mcr p15, 0, %0, c7, c6, 0" : : "r"(zero) : "memory");
}

static inline void cp15_dcache_flush_invd_all(void)
{
	uint32_t zero = 0;

	__asm__ volatile("mcr p15, 0, %0, c7, c14, 0" : : "r"(zero) : "memory");
}

static inline void cp15_dcache_clean_mva(void *mva)
{
	__asm__ volatile("mcr p15, 0, %0, c7, c10, 1" : : "r"(mva) : "memory");
}

static inline void cp15_dcache_invd_mva(void *mva)
{
	__asm__ volatile("mcr p15, 0, %0, c7, c6, 1" : : "r"(mva) : "memory");
}

static inline void cp15_dcache_flush_invd_mva(void *mva)
{
	__asm__ volatile("mcr p15, 0, %0, c7, c14, 1" : : "r"(mva) : "memory");
}

static inline void cp15_icache_invd_all(void)
{
	uint32_t zero = 0;

	__asm__ volatile("mcr p15, 0, %0, c7, c5, 0" : : "r"(zero) : "memory");
}

static inline void cp15_icache_invd_mva(void *mva)
{
	__asm__ volatile("mcr p15, 0, %0, c7, c5, 1" : : "r"(mva) : "memory");
}

#ifdef CONFIG_DCACHE

size_t arch_dcache_line_size_get(void)
{
	return ARM1176_CACHE_LINE_BYTES;
}

void arch_dcache_enable(void)
{
	uint32_t v = __get_SCTLR();

	if (v & SCTLR_C_Msk) {
		cp15_dcache_flush_invd_all();
		barrier_dsync_fence_full();
		return;
	}

	cp15_dcache_invd_all();
	barrier_dsync_fence_full();
	__set_SCTLR(v | SCTLR_C_Msk);
	barrier_isync_fence_full();
}

void arch_dcache_disable(void)
{
	uint32_t v;

	cp15_dcache_flush_invd_all();
	barrier_dsync_fence_full();

	v = __get_SCTLR();
	__set_SCTLR(v & ~SCTLR_C_Msk);
	barrier_isync_fence_full();
}

int arch_dcache_flush_all(void)
{
	cp15_dcache_clean_all();
	barrier_dsync_fence_full();
	return 0;
}

int arch_dcache_invd_all(void)
{
	cp15_dcache_invd_all();
	barrier_dsync_fence_full();
	return 0;
}

int arch_dcache_flush_and_invd_all(void)
{
	cp15_dcache_flush_invd_all();
	barrier_dsync_fence_full();
	return 0;
}

int arch_dcache_flush_range(void *start_addr, size_t size)
{
	ARG_UNUSED(start_addr);
	ARG_UNUSED(size);

	/* DIAGNOSTIC (Zero W WiFi bring-up): fall back to clean-all for
	 * parity with invd_range. If DMA then produces correct data, every
	 * MVA-by-line op is broken and needs a different encoding.
	 */
	cp15_dcache_clean_all();
	barrier_dsync_fence_full();
	return 0;
}

int arch_dcache_invd_range(void *start_addr, size_t size)
{
	ARG_UNUSED(start_addr);
	ARG_UNUSED(size);

	/* DIAGNOSTIC (Zero W WiFi bring-up 2026-07-01): the MVA-per-line
	 * loop appears to leave stale L1 cache lines behind (chipid reads
	 * return the pre-DMA sentinel). Fall back to clean+invalidate the
	 * whole D-cache to isolate the bug. If this fixes SDIO DMA reads,
	 * the MVA loop is broken; if it doesn't, DMA is writing somewhere
	 * the CPU can't see (dma_bcm2835 bus-address translation).
	 */
	cp15_dcache_flush_invd_all();
	barrier_dsync_fence_full();
	return 0;
}

int arch_dcache_flush_and_invd_range(void *start_addr, size_t size)
{
	ARG_UNUSED(start_addr);
	ARG_UNUSED(size);

	/* DIAGNOSTIC (Zero W WiFi bring-up): parity with invd_range. */
	cp15_dcache_flush_invd_all();
	barrier_dsync_fence_full();
	return 0;
}

#endif /* CONFIG_DCACHE */

#ifdef CONFIG_ICACHE

size_t arch_icache_line_size_get(void)
{
	return ARM1176_CACHE_LINE_BYTES;
}

void arch_icache_enable(void)
{
	uint32_t v = __get_SCTLR();

	if (v & SCTLR_I_Msk) {
		cp15_icache_invd_all();
		barrier_isync_fence_full();
		return;
	}
	cp15_icache_invd_all();
	__set_SCTLR(v | SCTLR_I_Msk);
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
	cp15_icache_invd_all();
	barrier_isync_fence_full();
	return 0;
}

int arch_icache_flush_and_invd_all(void)
{
	return -ENOTSUP;
}

int arch_icache_flush_range(void *start_addr, size_t size)
{
	ARG_UNUSED(start_addr);
	ARG_UNUSED(size);
	return -ENOTSUP;
}

int arch_icache_invd_range(void *start_addr, size_t size)
{
	uintptr_t addr = (uintptr_t)start_addr & ~(ARM1176_CACHE_LINE_BYTES - 1);
	uintptr_t end = (uintptr_t)start_addr + size;

	while (addr < end) {
		cp15_icache_invd_mva((void *)addr);
		addr += ARM1176_CACHE_LINE_BYTES;
	}
	barrier_isync_fence_full();
	return 0;
}

int arch_icache_flush_and_invd_range(void *start_addr, size_t size)
{
	ARG_UNUSED(start_addr);
	ARG_UNUSED(size);
	return -ENOTSUP;
}

#endif /* CONFIG_ICACHE */

void arch_cache_init(void)
{
	/* Caches are enabled by the MMU init path once the tables are up;
	 * there is nothing extra to configure here.
	 */
}
