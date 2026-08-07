/*
 * Copyright 2025-2026 NXP
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
#include <zephyr/cache.h>
#include <fsl_power.h>
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
	SystemInit();

#ifdef CONFIG_SOC_MCXE31B
	enable_sram_extra_latency(true);
#else
	enable_sram_extra_latency(false);
#endif
	/* Enable I/DCache */
	sys_cache_instr_enable();
	sys_cache_data_enable();
}

void soc_late_init_hook(void)
{
	if (POWER_ExitFromStandbyMode()) {
		/* Re-latch IO controls before application output after standby reset. */
		DCM_GPR->DCMRWF1 |= (uint32_t)DCM_GPR_DCMRWF1_STANDBY_IO_CONFIG_MASK;
	}
}
