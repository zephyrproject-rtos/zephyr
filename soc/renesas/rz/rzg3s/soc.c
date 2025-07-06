/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Renesas RZ/G3S Group
 */

#include <zephyr/init.h>
#include <bsp_api.h>

/* System core clock is set to 250 MHz by IPL of A55 */
uint32_t SystemCoreClock = 250000000;

void soc_early_init_hook(void)
{
	bsp_clock_init();

	/* This delay is required to wait for the A55 to complete its setting first before */
	/* UART initialization of M33 */
	R_BSP_SoftwareDelay(CONFIG_UART_RENESAS_RZG_INIT_DELAY_MS, BSP_DELAY_UNITS_MILLISECONDS);
}
