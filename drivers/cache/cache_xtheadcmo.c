/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/cache.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT xuantie_xtheadcmo

LOG_MODULE_REGISTER(cache_xtheadcmo, CONFIG_CACHE_LOG_LEVEL);

struct cache_config {
	uint32_t icache_line_size;
	uint32_t dcache_line_size;
};

static struct cache_config cache_cfg = {
	.icache_line_size = DT_INST_PROP(0, icache_line_size),
	.dcache_line_size = DT_INST_PROP(0, dcache_line_size),
};

int cache_data_invd_all(void)
{
	__asm__ volatile (
		"fence\n"
		/* th.dcache.iall */
		".insn 0x20000B\n"
		"fence\n"
	);

	return 0;
}

static void cache_invalidate_dcache_line(uintptr_t address_in)
{
	register uintptr_t address __asm__("a3") = address_in;

	__asm__ volatile (
		/* th.dcache.ipa a3*/
		".insn 0x2A6800B\n"
		:
		: "r"(address)
	);
}

int cache_data_invd_range(void *addr_in, size_t size)
{
	uintptr_t addr = (uintptr_t)addr_in;

	__asm__ volatile (
		"fence\n"
	);
	for (uintptr_t i = addr; i < addr + size; i += cache_cfg.dcache_line_size) {
		cache_invalidate_dcache_line(i);
	}
	__asm__ volatile (
		"fence\n"
	);

	return 0;
}

int cache_instr_invd_all(void)
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

static void cache_invalidate_icache_line(uintptr_t address_in)
{
	register uintptr_t address __asm__("a3") = address_in;

	__asm__ volatile (
		/* th.icache.ipa a3*/
		".insn 0x386800B\n"
		:
		: "r"(address)
	);
}

int cache_instr_invd_range(void *addr_in, size_t size)
{
	uintptr_t addr = (uintptr_t)addr_in;

	__asm__ volatile (
		"fence\n"
		"fence.i\n"
	);
	for (uintptr_t i = addr; i < addr + size; i += cache_cfg.icache_line_size) {
		cache_invalidate_icache_line(i);
	}
	__asm__ volatile (
		"fence\n"
		"fence.i\n"
	);

	return 0;
}

int cache_data_flush_all(void)
{
	__asm__ volatile (
		"fence\n"
		/* th.dcache.call */
		".insn 0x10000B\n"
		"fence\n"
	);

	return 0;
}

static void cache_clean_dcache_line(uintptr_t address_in)
{
	register uintptr_t address __asm__("a3") = address_in;

	__asm__ volatile (
		/* th.dcache.cpa a3*/
		".insn 0x296800B\n"
		:
		: "r"(address)
	);
}

int cache_data_flush_range(void *addr_in, size_t size)
{
	uintptr_t addr = (uintptr_t)addr_in;

	__asm__ volatile (
		"fence\n"
	);
	for (uintptr_t i = addr; i < addr + size; i += cache_cfg.dcache_line_size) {
		cache_clean_dcache_line(i);
	}
	__asm__ volatile (
		"fence\n"
	);

	return 0;
}

int cache_data_flush_and_invd_all(void)
{
	__asm__ volatile (
		"fence\n"
		/* th.dcache.ciall */
		".insn 0x30000B\n"
		"fence\n"
	);

	return 0;
}

static void cache_clean_invalidate_dcache_line(uintptr_t address_in)
{
	register uintptr_t address __asm__("a3") = address_in;

	__asm__ volatile (
		/* th.dcache.cipa a3*/
		".insn 0x2B6800B\n"
		:
		: "r"(address)
	);
}

int cache_data_flush_and_invd_range(void *addr_in, size_t size)
{
	uintptr_t addr = (uintptr_t)addr_in;

	__asm__ volatile (
		"fence\n"
	);
	for (uintptr_t i = addr; i < addr + size; i += cache_cfg.dcache_line_size) {
		cache_clean_invalidate_dcache_line(i);
	}
	__asm__ volatile (
		"fence\n"
	);

	return 0;
}

int cache_instr_flush_all(void)
{
	return -ENOTSUP;
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

int cache_instr_flush_and_invd_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}
