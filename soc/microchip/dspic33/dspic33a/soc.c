/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>

void soc_early_init_hook(void)
{
	/* Intentionally empty: This system dosent use SOC level init.
	 * SOC level init for dspic33a family of chips
	 */
}
