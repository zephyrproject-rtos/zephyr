/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>

void board_early_init_hook(void)
{
	/* Intentionally empty: This system dosent use Board level init.
	 * Board level init for dspic33a_curiosity
	 */
}
