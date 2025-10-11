/*
 * Copyright (c) 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/cache.h>

/* "a Hardware CSR" */
#define THEAD_MHCR "0x7C1"

void arch_icache_enable(void)
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

void arch_dcache_enable(void)
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

void arch_icache_disable(void)
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

void arch_dcache_disable(void)
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
