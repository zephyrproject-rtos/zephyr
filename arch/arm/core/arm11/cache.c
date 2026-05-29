/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM1176JZF-S L1-cache maintenance operations.
 *
 * Uses direct CP15 MCR instructions per the ARM1176JZF-S TRM.  CMSIS L1C_*
 * helpers are not used here because they rely on ARMv7-only CLIDR/DSB/ISB
 * encodings that are not accepted by the arm1176jzf-s assembler target.
 */

#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/sys/barrier.h>

/* Cache Type Register field for DminLine */
#define CTR_DMINLINE_SHIFT	16
#define CTR_DMINLINE_MASK	BIT_MASK(4)

#ifdef CONFIG_DCACHE

/* Forward declarations for functions called before their definition. */
int arch_dcache_invd_all(void);
int arch_dcache_flush_and_invd_all(void);

static size_t dcache_line_size;

static void dcache_clean_mva(void *addr)
{
	/* Clean data cache entry by MVA. */
	__asm__ volatile("mcr p15, 0, %0, c7, c10, 1" : : "r"(addr) : "memory");
}

static void dcache_invd_mva(void *addr)
{
	/* Invalidate data cache entry by MVA. */
	__asm__ volatile("mcr p15, 0, %0, c7, c6, 1" : : "r"(addr) : "memory");
}

static void dcache_clean_invd_mva(void *addr)
{
	/* Clean and invalidate data cache entry by MVA. */
	__asm__ volatile("mcr p15, 0, %0, c7, c14, 1" : : "r"(addr) : "memory");
}

size_t arch_dcache_line_size_get(void)
{
	uint32_t val;
	uint32_t dminline;

	if (!dcache_line_size) {
		val = read_sysreg(ctr);
		dminline = (val >> CTR_DMINLINE_SHIFT) & CTR_DMINLINE_MASK;
		dcache_line_size = 4 << dminline;
	}

	return dcache_line_size;
}

void arch_dcache_enable(void)
{
	uint32_t val = __get_SCTLR();

	if (val & SCTLR_C_Msk) {
		arch_dcache_flush_and_invd_all();
		return;
	}

	arch_dcache_invd_all();

	val |= SCTLR_C_Msk;
	barrier_dsync_fence_full();
	__set_SCTLR(val);
	barrier_isync_fence_full();
}

void arch_dcache_disable(void)
{
	uint32_t val;

	arch_dcache_flush_and_invd_all();

	val = __get_SCTLR();
	val &= ~SCTLR_C_Msk;
	barrier_dsync_fence_full();
	__set_SCTLR(val);
	barrier_isync_fence_full();
}

int arch_dcache_flush_all(void)
{
	/* Clean entire data cache. */
	__asm__ volatile("mcr p15, 0, %0, c7, c10, 0" : : "r"(0) : "memory");
	barrier_dsync_fence_full();
	return 0;
}

int arch_dcache_invd_all(void)
{
	/* Invalidate entire data cache. */
	__asm__ volatile("mcr p15, 0, %0, c7, c6, 0" : : "r"(0) : "memory");
	barrier_dsync_fence_full();
	return 0;
}

int arch_dcache_flush_and_invd_all(void)
{
	/* Clean and invalidate entire data cache. */
	__asm__ volatile("mcr p15, 0, %0, c7, c14, 0" : : "r"(0) : "memory");
	barrier_dsync_fence_full();
	return 0;
}

int arch_dcache_flush_range(void *start_addr, size_t size)
{
	size_t line_size;
	uintptr_t addr = (uintptr_t)start_addr;
	uintptr_t end_addr = addr + size;

	line_size = arch_dcache_line_size_get();
	addr &= ~(line_size - 1);

	while (addr < end_addr) {
		dcache_clean_mva((void *)addr);
		addr += line_size;
	}

	barrier_dmem_fence_full();
	return 0;
}

int arch_dcache_invd_range(void *start_addr, size_t size)
{
	size_t line_size;
	uintptr_t addr = (uintptr_t)start_addr;
	uintptr_t end_addr = addr + size;

	line_size = arch_dcache_line_size_get();

	if (end_addr & (line_size - 1)) {
		end_addr &= ~(line_size - 1);
		dcache_clean_invd_mva((void *)end_addr);
	}

	if (addr & (line_size - 1)) {
		addr &= ~(line_size - 1);
		if (addr == end_addr) {
			goto done;
		}
		dcache_clean_invd_mva((void *)addr);
		addr += line_size;
	}

	addr &= ~(line_size - 1);

	while (addr < end_addr) {
		dcache_invd_mva((void *)addr);
		addr += line_size;
	}

done:
	barrier_dmem_fence_full();
	return 0;
}

int arch_dcache_flush_and_invd_range(void *start_addr, size_t size)
{
	size_t line_size;
	uintptr_t addr = (uintptr_t)start_addr;
	uintptr_t end_addr = addr + size;

	line_size = arch_dcache_line_size_get();
	addr &= ~(line_size - 1);

	while (addr < end_addr) {
		dcache_clean_invd_mva((void *)addr);
		addr += line_size;
	}

	barrier_dmem_fence_full();
	return 0;
}

#endif /* CONFIG_DCACHE */

#ifdef CONFIG_ICACHE

/* Forward declaration. */
int arch_icache_invd_all(void);

void arch_icache_enable(void)
{
	uint32_t val = __get_SCTLR();

	if (val & SCTLR_I_Msk) {
		arch_icache_invd_all();
		return;
	}

	arch_icache_invd_all();
	__set_SCTLR(val | SCTLR_I_Msk);
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
	/* Invalidate entire I-cache. */
	__asm__ volatile("mcr p15, 0, %0, c7, c5, 0" : : "r"(0) : "memory");
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
	ARG_UNUSED(start_addr);
	ARG_UNUSED(size);
	return arch_icache_invd_all();
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
	/* ARM11 cache setup is handled by the explicit enable/disable paths. */
}
