/*
 * Copyright (c) 2023 Rivos Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>

#define PWRMGR_BASE (DT_REG_ADDR(DT_NODELABEL(pwrmgr)))
#define RV_TIMER_BASE (DT_REG_ADDR(DT_NODELABEL(mtimer)))

static int soc_opentitan_init(void)
{
	/* Enable the watchdog reset (bit 1). */
	sys_write32(2u, PWRMGR_BASE + PWRMGR_RESET_EN_REG_OFFSET);
	/* Write CFG_CDC_SYNC to commit change. */
	sys_write32(1u, PWRMGR_BASE + PWRMGR_CFG_CDC_SYNC_REG_OFFSET);
	/* Poll CFG_CDC_SYNC register until it reads 0. */
	while (sys_read32(PWRMGR_BASE + PWRMGR_CFG_CDC_SYNC_REG_OFFSET)) {
	}

	/* Initialize the Machine Timer, so it behaves as a regular one. */
	sys_write32(1u, RV_TIMER_BASE + RV_TIMER_CTRL_REG_OFFSET);
	/* Enable timer interrupts. */
	sys_write32(1u, RV_TIMER_BASE + RV_TIMER_INTR_ENABLE_REG_OFFSET);
	return 0;
}
SYS_INIT(soc_opentitan_init, PRE_KERNEL_1, 0);
