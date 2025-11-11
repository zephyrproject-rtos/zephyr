/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/cache.h>

int arch_dcache_invd_all(void)
{
	__asm__ volatile (
		"fence\n"
		/* th.dcache.iall */
		".insn 0x20000B\n"
		"fence\n"
	);

	return 0;
}

static void arch_cache_invalidate_dcache_line(uintptr_t address_in)
{
	register uintptr_t address __asm__("a3") = address_in;

	__asm__ volatile (
		/* th.dcache.ipa a3*/
		".insn 0x2A6800B\n"
		:
		: "r"(address)
	);
}

int arch_dcache_invd_range(void *addr_in, size_t size)
{
	uintptr_t addr = (uintptr_t)addr_in;

	__asm__ volatile (
		"fence\n"
	);
	for (uintptr_t i = addr; i < ROUND_UP(addr + size, CONFIG_DCACHE_LINE_SIZE);
	     i += CONFIG_DCACHE_LINE_SIZE) {
		arch_cache_invalidate_dcache_line(i);
	}
	__asm__ volatile (
		"fence\n"
	);

	return 0;
}

int arch_icache_invd_all(void)
{
	__asm__ volatile (
		"fence\n"
		"fence.i\n"
		/* th.icache.iall */
		".insn 0x100000B\n"
		"fence\n"
		"fence.i\n"
	);

	return 0;
}

static void arch_cache_invalidate_icache_line(uintptr_t address_in)
{
	register uintptr_t address __asm__("a3") = address_in;

	__asm__ volatile (
		/* th.icache.ipa a3*/
		".insn 0x386800B\n"
		:
		: "r"(address)
	);
}

int arch_icache_invd_range(void *addr_in, size_t size)
{
	uintptr_t addr = (uintptr_t)addr_in;

	__asm__ volatile (
		"fence\n"
		"fence.i\n"
	);
	for (uintptr_t i = addr; i < ROUND_UP(addr + size, CONFIG_ICACHE_LINE_SIZE);
	     i += CONFIG_ICACHE_LINE_SIZE) {
		arch_cache_invalidate_icache_line(i);
	}
	__asm__ volatile (
		"fence\n"
		"fence.i\n"
	);

	return 0;
}

int arch_dcache_flush_all(void)
{
	__asm__ volatile (
		"fence\n"
		/* th.dcache.call */
		".insn 0x10000B\n"
		"fence\n"
	);

	return 0;
}

static void arch_cache_clean_dcache_line(uintptr_t address_in)
{
	register uintptr_t address __asm__("a3") = address_in;

	__asm__ volatile (
		/* th.dcache.cpa a3*/
		".insn 0x296800B\n"
		:
		: "r"(address)
	);
}

int arch_dcache_flush_range(void *addr_in, size_t size)
{
	uintptr_t addr = (uintptr_t)addr_in;

	__asm__ volatile (
		"fence\n"
	);
	for (uintptr_t i = addr; i < ROUND_UP(addr + size, CONFIG_DCACHE_LINE_SIZE);
	     i += CONFIG_DCACHE_LINE_SIZE) {
		arch_cache_clean_dcache_line(i);
	}
	__asm__ volatile (
		"fence\n"
	);

	return 0;
}

int arch_dcache_flush_and_invd_all(void)
{
	__asm__ volatile (
		"fence\n"
		/* th.dcache.ciall */
		".insn 0x30000B\n"
		"fence\n"
	);

	return 0;
}

static void arch_cache_clean_invalidate_dcache_line(uintptr_t address_in)
{
	register uintptr_t address __asm__("a3") = address_in;

	__asm__ volatile (
		/* th.dcache.cipa a3*/
		".insn 0x2B6800B\n"
		:
		: "r"(address)
	);
}

int arch_dcache_flush_and_invd_range(void *addr_in, size_t size)
{
	uintptr_t addr = (uintptr_t)addr_in;

	__asm__ volatile (
		"fence\n"
	);
	for (uintptr_t i = addr; i < ROUND_UP(addr + size, CONFIG_DCACHE_LINE_SIZE);
	     i += CONFIG_DCACHE_LINE_SIZE) {
		arch_cache_clean_invalidate_dcache_line(i);
	}
	__asm__ volatile (
		"fence\n"
	);

	return 0;
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
