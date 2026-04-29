/*
 * Copyright (c) 2023 Rivos Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>

/*
 * RV_TIMER peripheral base address.
 *
 * The `riscv,machine-timer` DT node (mtimer) uses `reg` to expose the mtime
 * and mtimecmp register addresses (0x40100110 and 0x40100118) as required by
 * the standard timer driver. DT_REG_ADDR(DT_NODELABEL(mtimer)) therefore
 * returns 0x40100110, not the peripheral base 0x40100000. Using the DT macro
 * here would place CTRL (offset 0x004) and INTR_ENABLE (offset 0x100) at
 * wrong addresses, causing a store access fault in soc_early_init_hook.
 * The peripheral base is taken directly from the OpenTitan Earl Grey address
 * map (hw/top_earlgrey/sw/autogen/top_earlgrey.h: TOP_EARLGREY_RV_TIMER_BASE_ADDR).
 */
#define RV_TIMER_BASE                   0x40100000u
#define RV_TIMER_CTRL_REG_OFFSET        0x004
#define RV_TIMER_INTR_ENABLE_REG_OFFSET 0x100

void soc_early_init_hook(void)
{
	/* Enable the rv_timer hart and timer interrupt. */
	sys_write32(1u, RV_TIMER_BASE + RV_TIMER_CTRL_REG_OFFSET);
	sys_write32(1u, RV_TIMER_BASE + RV_TIMER_INTR_ENABLE_REG_OFFSET);
}
