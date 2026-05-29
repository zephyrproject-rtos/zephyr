/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for nxp_mcxl family
 *
 * This module provides routines to initialize and support board-level
 * hardware for the nxp_mcxl family.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>

void soc_reset_hook(void)
{
	SystemInit();
}
