/*
 * Copyright 2026 Hiyajomaho Num9
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <cmsis_core.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/util.h>

#define RK3506_SCTLR_BOOT_STALE_BITS \
	(SCTLR_M_Msk | SCTLR_A_Msk | SCTLR_C_Msk | SCTLR_I_Msk | HIVECS)

static void rk3506_set_vector_base(void)
{
	uint32_t sctlr = __get_SCTLR();

	sctlr &= ~HIVECS;
	__set_SCTLR(sctlr);
	barrier_isync_fence_full();

	__set_VBAR((uint32_t)_vector_start & VBAR_MASK);
	barrier_isync_fence_full();
}

void soc_reset_hook(void)
{
	uint32_t sctlr;

	sctlr = __get_SCTLR();
	if ((sctlr & SCTLR_C_Msk) != 0U) {
		L1C_CleanInvalidateDCacheAll();
	}

	barrier_dsync_fence_full();
	sctlr &= ~RK3506_SCTLR_BOOT_STALE_BITS;
	__set_SCTLR(sctlr);
	barrier_isync_fence_full();

	__set_TLBIALL(0);
	__set_ICIALLU(0);
	__set_BPIALL(0);
	barrier_dsync_fence_full();
	barrier_isync_fence_full();

	rk3506_set_vector_base();
}

void relocate_vector_table(void)
{
	/*
	 * RK3506 boots Zephyr from DDR. Physical address 0 is not writable RAM,
	 * so use VBAR instead of copying the vector table to address 0.
	 */
	rk3506_set_vector_base();
}
