/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for nxp_mcxe platform
 *
 * This module provides routines to initialize and support board-level
 * hardware for the nxp_mcxe platform.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include "soc.h"

/**
 *
 * @brief Perform basic hardware initialization
 *
 * Initialize the interrupt controller device drivers.
 * Also initialize the counter device driver, if required.
 *
 * @return 0
 */
void soc_early_init_hook(void)
{
	enable_sram_extra_latency(true);
}

#ifdef CONFIG_SOC_RESET_HOOK
void soc_reset_hook(void)
{
	SystemInit();
}
#endif /* CONFIG_SOC_RESET_HOOK */
