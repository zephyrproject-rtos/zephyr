/*
 * Copyright 2020 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/arch/cpu.h>

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int viper_init(const struct device *arg)
{
	uint32_t key;
	uint32_t data;

	ARG_UNUSED(arg);

	key = irq_lock();

	/* pcie pmon lite init */
	data = sys_read32(LS_ICFG_PMON_LITE_CLK_CTRL);
	data |= PCIE_PMON_LITE_CLK_ENABLE;
	sys_write32(data, LS_ICFG_PMON_LITE_CLK_CTRL);

	data = sys_read32(LS_ICFG_PMON_LITE_SW_RESETN);
	data |= PCIE_PMON_LITE_SW_RESETN;
	sys_write32(data, LS_ICFG_PMON_LITE_SW_RESETN);

	irq_unlock(key);

	return 0;
}

SYS_INIT(viper_init, PRE_KERNEL_1, 0);
