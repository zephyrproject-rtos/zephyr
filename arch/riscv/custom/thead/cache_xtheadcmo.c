/*
 * Copyright (c) 2025-2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/cache.h>

int arch_dcache_invd_all(void)
{
#ifdef CONFIG_DCACHE
	__asm__ volatile (
		"fence\n"
		"fence.i\n"
		"th.dcache.iall\n"
		"fence\n"
		"fence.i\n"
		"th.sync.i\n"
		::: "memory"
	);

	return 0;
#else
	return -ENOTSUP;
#endif
}

#ifdef CONFIG_DCACHE
static void arch_cache_invalidate_dcache_line(uintptr_t address)
{
	__asm__ volatile (
		"th.dcache.ipa %0\n"
		:
		: "r"(address)
		: "memory"
	);
}
#endif

int arch_dcache_invd_range(void *addr_in, size_t size)
{
#ifdef CONFIG_DCACHE
	uintptr_t addr = (uintptr_t)addr_in;

	__asm__ volatile (
		"fence\n"
		"fence.i\n"
		::: "memory"
	);
	for (uintptr_t i = addr; i < ROUND_UP(addr + size, CONFIG_DCACHE_LINE_SIZE);
	     i += CONFIG_DCACHE_LINE_SIZE) {
		arch_cache_invalidate_dcache_line(i);
	}
	__asm__ volatile (
		"fence\n"
		"fence.i\n"
		"th.sync.i\n"
		::: "memory"
	);

	return 0;
#else
	return -ENOTSUP;
#endif
}

int arch_icache_invd_all(void)
{
#ifdef CONFIG_ICACHE
	__asm__ volatile (
		"fence\n"
		"fence.i\n"
		"th.icache.iall\n"
		"fence\n"
		"fence.i\n"
		"th.sync.i\n"
		::: "memory"
	);

	return 0;
#else
	return -ENOTSUP;
#endif
}

#ifdef CONFIG_ICACHE
static void arch_cache_invalidate_icache_line(uintptr_t address)
{
	__asm__ volatile (
		"th.icache.ipa %0\n"
		:
		: "r"(address)
		: "memory"
	);
}
#endif

int arch_icache_invd_range(void *addr_in, size_t size)
{
#ifdef CONFIG_ICACHE
	uintptr_t addr = (uintptr_t)addr_in;

	__asm__ volatile (
		"fence\n"
		"fence.i\n"
		::: "memory"
	);
	for (uintptr_t i = addr; i < ROUND_UP(addr + size, CONFIG_ICACHE_LINE_SIZE);
	     i += CONFIG_ICACHE_LINE_SIZE) {
		arch_cache_invalidate_icache_line(i);
	}
	__asm__ volatile (
		"fence\n"
		"fence.i\n"
		"th.sync.i\n"
		::: "memory"
	);

	return 0;
#else
	return -ENOTSUP;
#endif
}

int arch_dcache_flush_all(void)
{
#ifdef CONFIG_DCACHE
	__asm__ volatile (
		"fence\n"
		"fence.i\n"
		"th.dcache.call\n"
		"fence\n"
		"fence.i\n"
		"th.sync.i\n"
		::: "memory"
	);

	return 0;
#else
	return -ENOTSUP;
#endif
}

#ifdef CONFIG_DCACHE
static void arch_cache_clean_dcache_line(uintptr_t address)
{
	__asm__ volatile (
		"th.dcache.cpa %0\n"
		:
		: "r"(address)
		: "memory"
	);
}
#endif

int arch_dcache_flush_range(void *addr_in, size_t size)
{
#ifdef CONFIG_DCACHE
	uintptr_t addr = (uintptr_t)addr_in;

	__asm__ volatile (
		"fence\n"
		"fence.i\n"
		::: "memory"
	);
	for (uintptr_t i = addr; i < ROUND_UP(addr + size, CONFIG_DCACHE_LINE_SIZE);
	     i += CONFIG_DCACHE_LINE_SIZE) {
		arch_cache_clean_dcache_line(i);
	}
	__asm__ volatile (
		"fence\n"
		"fence.i\n"
		"th.sync.i\n"
		::: "memory"
	);

	return 0;
#else
	return -ENOTSUP;
#endif
}

int arch_dcache_flush_and_invd_all(void)
{
#ifdef CONFIG_DCACHE
	__asm__ volatile (
		"fence\n"
		"fence.i\n"
		"th.dcache.ciall\n"
		"fence\n"
		"fence.i\n"
		"th.sync.i\n"
		::: "memory"
	);

	return 0;
#else
	return -ENOTSUP;
#endif
}

#ifdef CONFIG_DCACHE
static void arch_cache_clean_invalidate_dcache_line(uintptr_t address)
{
	__asm__ volatile (
		"th.dcache.cipa %0\n"
		:
		: "r"(address)
		: "memory"
	);
}
#endif

int arch_dcache_flush_and_invd_range(void *addr_in, size_t size)
{
#ifdef CONFIG_DCACHE
	uintptr_t addr = (uintptr_t)addr_in;

	__asm__ volatile (
		"fence\n"
		"fence.i\n"
		::: "memory"
	);
	for (uintptr_t i = addr; i < ROUND_UP(addr + size, CONFIG_DCACHE_LINE_SIZE);
	     i += CONFIG_DCACHE_LINE_SIZE) {
		arch_cache_clean_invalidate_dcache_line(i);
	}
	__asm__ volatile (
		"fence\n"
		"fence.i\n"
		"th.sync.i\n"
		::: "memory"
	);

	return 0;
#else
	return -ENOTSUP;
#endif
}

int arch_icache_flush_all(void)
{
	return -ENOTSUP;
}

int arch_icache_flush_and_invd_all(void)
{
	return -ENOTSUP;
}

int arch_icache_flush_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

int arch_icache_flush_and_invd_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}
