/*
 * Copyright (c) 2023 Rivos Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>

/* OpenTitan power management regs. */
#define PWRMGR_BASE (DT_REG_ADDR(DT_NODELABEL(pwrmgr)))
#define PWRMGR_CFG_CDC_SYNC_REG_OFFSET  0x018
#define PWRMGR_RESET_EN_REG_OFFSET      0x02c
#define PWRMGR_RESET_EN_WDOG_SRC_MASK   0x002

/* Ibex timer registers. */
#define RV_TIMER_BASE (DT_REG_ADDR(DT_NODELABEL(mtimer)))
#define RV_TIMER_CTRL_REG_OFFSET        0x004
#define RV_TIMER_INTR_ENABLE_REG_OFFSET 0x100
#define RV_TIMER_CFG0_REG_OFFSET        0x10c
#define RV_TIMER_CFG0_PRESCALE_MASK     0xfff
#define RV_TIMER_CFG0_PRESCALE_OFFSET   0
#define RV_TIMER_CFG0_STEP_MASK         0xff
#define RV_TIMER_CFG0_STEP_OFFSET       16
#define RV_TIMER_LOWER0_OFFSET          0x110
#define RV_TIMER_COMPARE_LOWER0_OFFSET  0x118

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
