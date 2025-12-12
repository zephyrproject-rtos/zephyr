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

#define PERIF_STICKY_RST_OFFSET 0x88

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * Enable clocks not managed by corresponding device drivers.
 */
void soc_early_init_hook(void)
{
	const mem_addr_t swire_ctrl = DT_REG_ADDR(DT_NODELABEL(pinctrl_swire));
	const mem_addr_t perif_sticky_rst =
		DT_REG_ADDR(DT_NODELABEL(gcr)) + PERIF_STICKY_RST_OFFSET;
	uint32_t value;
	volatile uint32_t loops;

	/* Setup various clocks and wakeup sources */

	value = sys_read32(swire_ctrl);
	value &= ~(0x00800000); /* SWIRE power down */
	sys_write32(value, swire_ctrl);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(usb0), okay)
	value = sys_read32(perif_sticky_rst);
	value &= ~0x7; /* Assert usb0PhyRstn, usb0CoreRstn & usb0MahbRstn */
	sys_write32(value, perif_sticky_rst);

	value |= 1 << 0; /* De-assert usb0PhyRstn */
	sys_write32(value, perif_sticky_rst);

	/* Wait more than 45us */
	loops = 200;
	while (loops-- > 0) {
		arch_nop();
	}

	value |= 1 << 2 | 1 << 1; /* De-assert usb0CoreRstn & usb0MahbRstn  */
	sys_write32(value, perif_sticky_rst);
#endif /* usb0 */
}
