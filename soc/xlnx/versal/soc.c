/*
 * Copyright (c) 2025 Space Cubics Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <cmsis_core.h>
#include "soc.h"

void soc_reset_hook(void)
{
	uint32_t sctlr;

	/* Clear HIVECS bit to ensure exception vectors are at 0x00000000 */
	sctlr = __get_SCTLR();
	sctlr &= ~SCTLR_V_Msk;
	__set_SCTLR(sctlr);
}

void soc_early_init_hook(void)
{
	/* Enable caches */
	sys_cache_instr_enable();
	sys_cache_data_enable();
}
