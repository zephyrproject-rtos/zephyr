/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Renesas RZ/V2L Group
 */

#include <zephyr/init.h>
#include <bsp_api.h>

/* System core clock is set to 200 MHz after reset */
uint32_t SystemCoreClock = 200000000;

void soc_early_init_hook(void)
{
	bsp_clock_init();
}
