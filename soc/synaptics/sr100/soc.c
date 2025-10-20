/*
 * Copyright (c) 2025 Synaptics Incorporated.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for the Synaptics SR1xx SoC
 *
 * This module provides routines to initialize and support board-level hardware
 * for the Synaptics SR1xx SoC.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * Enable clocks not managed by corresponding device drivers.
 */
void soc_early_init_hook(void)
{
	const mem_addr_t swire_ctrl = DT_REG_ADDR(DT_NODELABEL(pinctrl_swire));
	uint32_t value;

	/* Setup various clocks and wakeup sources */

	value = sys_read32(swire_ctrl);
	value &= ~(0x00800000); /* Disable SWIRE power down */
	sys_write32(value, swire_ctrl);
}
