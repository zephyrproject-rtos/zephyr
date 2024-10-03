/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SoC initialization for Silicon Labs Series 2 products
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <em_chip.h>
#include <sl_device_init_dcdc.h>
#include <sl_clock_manager_init.h>
#include <sl_hfxo_manager.h>
#include <sl_power_manager.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

void soc_early_init_hook(void)
{
	/* Handle chip errata */
	CHIP_Init();

	if (DT_HAS_COMPAT_STATUS_OKAY(silabs_series2_dcdc)) {
		sl_device_init_dcdc();
	}
	sl_clock_manager_init();

	if (IS_ENABLED(CONFIG_PM)) {
		sl_power_manager_init();
		sl_hfxo_manager_init();
	}
}
