/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>

#define RET_SIZE DT_REG_SIZE(DT_NODELABEL(retention))
BUILD_ASSERT(RET_SIZE == 0 || RET_SIZE == KB(16) || RET_SIZE == KB(32) || RET_SIZE == KB(64),
	     "Unsupported retention-size");

void soc_early_init_hook(void)
{
	CFG_REGS->CFG_PMD1 = 0x200107e0U;
	CFG_REGS->CFG_PMD2 = 0x0U;
	CFG_REGS->CFG_PMD3 = 0xf73ffffU;

	/* CM4 SW Reset Enabled */
	CFG_REGS->CFG_MISCSTAT |= 0x8000;
}

void soc_late_init_hook(void)
{
	PMU_REGS->PMU_WCMSIZ = (PMU_REGS->PMU_WCMSIZ & ~PMU_WCMSIZ_SRAM1_SIZ_Msk);
	PMU_REGS->PMU_WCMSIZ |= PMU_WCMSIZ_SRAM1_SIZ(RET_SIZE / KB(16));
}
