/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/cache.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cache_xtheadcmo_bl61x, CONFIG_CACHE_LOG_LEVEL);

/* "a Hardware CSR" */
#define THEAD_MHCR "0x7C1"

void cache_instr_enable(void)
{
	uint32_t tmp;

	__asm__ volatile (
		"fence\n"
		"fence.i\n"
		/* th.icache.iall */
		".insn 0x100000B\n"
	);
	__asm__ volatile(
		"csrr %0, " THEAD_MHCR
		: "=r"(tmp));
	tmp |= (1 << 0);
	__asm__ volatile(
		"csrw " THEAD_MHCR ", %0"
		:
		: "r"(tmp));
	__asm__ volatile (
		"fence\n"
		"fence.i\n"
	);
}

void cache_data_enable(void)
{
	uint32_t tmp;

	__asm__ volatile (
		"fence\n"
		"fence.i\n"
		/* th.dcache.iall */
		".insn 0x20000B\n"
	);
	__asm__ volatile(
		"csrr %0, " THEAD_MHCR
		: "=r"(tmp));
	tmp |= (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4);
	__asm__ volatile(
		"csrw " THEAD_MHCR ", %0"
		:
		: "r"(tmp));
	__asm__ volatile (
		"fence\n"
		"fence.i\n"
	);
}

void cache_instr_disable(void)
{
	uint32_t tmp;

	__asm__ volatile (
		"fence\n"
		"fence.i\n"
		/* th.icache.iall */
		".insn 0x100000B\n"
	);
	__asm__ volatile(
		"csrr %0, " THEAD_MHCR
		: "=r"(tmp));
	tmp &= ~(1 << 0);
	__asm__ volatile(
		"csrw " THEAD_MHCR ", %0"
		:
		: "r"(tmp));
	__asm__ volatile (
		"fence\n"
		"fence.i\n"
	);
}

void cache_data_disable(void)
{
	uint32_t tmp;

	__asm__ volatile (
		"fence\n"
		"fence.i\n"
		/* th.dcache.iall */
		".insn 0x20000B\n"
	);
	__asm__ volatile(
		"csrr %0, " THEAD_MHCR
		: "=r"(tmp));
	tmp &= ~((1 << 1) | (1 << 2) | (1 << 3) | (1 << 4));
	__asm__ volatile(
		"csrw " THEAD_MHCR ", %0"
		:
		: "r"(tmp));
	__asm__ volatile (
		"fence\n"
		"fence.i\n"
	);
}
