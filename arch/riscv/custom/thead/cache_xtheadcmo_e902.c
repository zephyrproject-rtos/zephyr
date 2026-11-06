/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/cache.h>

#include "cache_xtheadcmo_csr.h"

void arch_icache_enable(void)
{
	uint32_t tmp;

	__asm__ volatile (
		"fence\n"
		"fence.i\n"
		"th.icache.iall\n"
	);
	__asm__ volatile(
		"csrr %0, " THEAD_MHCR
		: "=r"(tmp));
	tmp |= THEAD_MHCR_IE;
	__asm__ volatile(
		"csrw " THEAD_MHCR ", %0"
		:
		: "r"(tmp));
	__asm__ volatile (
		"fence\n"
		"fence.i\n"
		::: "memory"
	);
}

void arch_icache_disable(void)
{
	uint32_t tmp;

	__asm__ volatile (
		"fence\n"
		"fence.i\n"
		"th.icache.iall\n"
	);
	__asm__ volatile(
		"csrr %0, " THEAD_MHCR
		: "=r"(tmp));
	tmp &= ~THEAD_MHCR_IE;
	__asm__ volatile(
		"csrw " THEAD_MHCR ", %0"
		:
		: "r"(tmp));
	__asm__ volatile (
		"fence\n"
		"fence.i\n"
		::: "memory"
	);
}
