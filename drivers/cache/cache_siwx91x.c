/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/cache.h>
#include <zephyr/sys/barrier.h>

#include "rsi_pll.h"

/*
 * The SiWx91x has an external AHB instruction cache for the QSPI flash. It
 * has no data cache. The cache clock is controlled via the M4CLK set/clear
 * register pair. There is no dedicated invalidate command: toggling the clock
 * gate implicitly clears all cached lines.
 */

static bool icache_is_enabled(void)
{
	return !!(M4CLK->CLK_ENABLE_SET_REG3 & ICACHE_ENABLE);
}

/* Instruction cache */

void cache_instr_enable(void)
{
	M4CLK->CLK_ENABLE_SET_REG3 = ICACHE_ENABLE;
}

void cache_instr_disable(void)
{
	M4CLK->CLK_ENABLE_CLEAR_REG3 = ICACHE_ENABLE;
	barrier_isync_fence_full();
}

int cache_instr_flush_all(void)
{
	/* Instruction cache is read-only: no dirty lines to write back */
	return -ENOTSUP;
}

int cache_instr_invd_all(void)
{
	if (!icache_is_enabled()) {
		return -EAGAIN;
	}

	/*
	 * Toggle the clock gate to clear all cached lines. The cache restarts
	 * empty after re-enabling. Follow with an ISB to flush the Cortex-M4
	 * instruction pipeline.
	 */
	M4CLK->CLK_ENABLE_CLEAR_REG3 = ICACHE_ENABLE;
	M4CLK->CLK_ENABLE_SET_REG3 = ICACHE_ENABLE;
	barrier_isync_fence_full();

	return 0;
}

int cache_instr_flush_and_invd_all(void)
{
	/* Instruction cache is read-only, so flush is a no-op before invalidate */
	return cache_instr_invd_all();
}

int cache_instr_flush_range(void *addr, size_t size)
{
	return -ENOTSUP;
}

int cache_instr_invd_range(void *addr, size_t size)
{
	/* No per-line control available: fall back to full invalidation */
	return cache_instr_invd_all();
}

int cache_instr_flush_and_invd_range(void *addr, size_t size)
{
	return cache_instr_invd_range(addr, size);
}
