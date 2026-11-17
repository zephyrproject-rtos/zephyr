/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/platform/hooks.h>
#include <zephyr/sys/util.h>

/* CNTControl frame of the SSE-700 REFCLK counter (Armv8 memory-mapped counter) */
#define REFCLK_CNT_CONTROL_BASE 0x1A200000UL
#define CNTCR                   0x000
#define CNTCR_EN                BIT(0)

void soc_prep_hook(void)
{
	mem_addr_t cntcr = REFCLK_CNT_CONTROL_BASE + CNTCR;

	/*
	 * The REFCLK counter drives the Generic Timer, and is disabled at
	 * reset; TF-A enables it when it is the one booting the APSS. This runs
	 * on the primary core with the MMU still off, so the physical address
	 * is used.
	 */
	sys_write32(sys_read32(cntcr) | CNTCR_EN, cntcr);
}
