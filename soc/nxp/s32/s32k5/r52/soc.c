/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/barrier.h>
#include <zephyr/cache.h>

#include <OsIf.h>

void soc_reset_hook(void)
{
	/* Enable peripheral port access at EL1 and EL0 */
	__asm__ volatile("mrc p15, 0, r0, c15, c0, 0\n");
	__asm__ volatile("orr r0, #1\n");
	__asm__ volatile("mcr p15, 0, r0, c15, c0, 0\n");
	barrier_dsync_fence_full();
	barrier_isync_fence_full();

	/*
	 * Take exceptions in Arm mode because Zephyr ASM code for Cortex-R Aarch32
	 * is written for Arm
	 */
	__set_SCTLR(__get_SCTLR() & ~SCTLR_TE_Msk);
}

void soc_early_init_hook(void)
{
	sys_cache_instr_enable();
	sys_cache_data_enable();

	OsIf_Init(NULL);
}
