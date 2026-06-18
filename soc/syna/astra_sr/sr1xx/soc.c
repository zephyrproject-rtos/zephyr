/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Synaptics Incorporated
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

#include <zephyr/cache.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>

#define AON_CONFIG 0x50350000

#define LP_CLKRST     0xb5007000
#define LP_CLK_ENABLE 0x00
#define UART_PORT_SEL 0x34

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * Enable clocks not managed by corresponding device drivers.
 */
void soc_early_init_hook(void)
{
	const mem_addr_t swire_ctrl = DT_REG_ADDR(DT_NODELABEL(pinctrl_swire));
	uint32_t value;

	/* Enable caches */
	sys_cache_instr_enable();
#ifdef CONFIG_SOC_SR100_M55
	sys_cache_data_enable();

	/* Enable Loop and branch info cache */
	SCB->CCR |= SCB_CCR_LOB_Msk;
	__DSB();
	__ISB();
#endif

	/* Setup various clocks and wakeup sources */

	value = sys_read32(swire_ctrl);
	value &= ~(0x00800000); /* Disable SWIRE power down */
	value |= 0x7b7bc;       /* Pull-up & increased drive-strength for I2C1 */
	sys_write32(value, swire_ctrl);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart_lp_rx_b), okay)
	/* Set UART port sel to 'B' (1) instead of 'A' (0) */
	sys_write32(0x1, LP_CLKRST + UART_PORT_SEL);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lp_gpio), okay)
	value = sys_read32(LP_CLKRST + LP_CLK_ENABLE);
	value |= BIT(17) | BIT(19) | BIT(20);
	sys_write32(value, LP_CLKRST + LP_CLK_ENABLE);
#endif
}

void soc_late_init_hook(void)
{
#ifdef CONFIG_SR100_RELEASE_M4_RESET
	/* Take M4 out of reset (AON_MAIN - AON_CONFIG/LPP_DEBUG_MODE = running) */
	uint32_t value = sys_read32(AON_CONFIG);

	value &= ~(0x3 << 8);
	sys_write32(value, AON_CONFIG);
#endif
}
