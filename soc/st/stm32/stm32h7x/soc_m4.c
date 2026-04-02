/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32H7 CM4 processor
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_cortex.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_system.h>
#include "stm32_hsem.h"

#include <cmsis_core.h>

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 */
void soc_early_init_hook(void)
{
	/* Enable ART Flash cache accelerator */
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_ART);
	LL_ART_SetBaseAddress(DT_REG_ADDR(DT_CHOSEN(zephyr_flash)));
	LL_ART_Enable();

	/* Enable hardware semaphore clock */
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_HSEM);

	/**
	 * Cortex-M7 is responsible for initializing the system.
	 *
	 * CM7 will start CM4 ("forced boot") at the end of system
	 * initialization if the core is not already running - in
	 * this scenario, we don't have to wait because the system
	 * is initialized. Otherwise, wait for CM7 to initialize the
	 * system before proceeding with the boot process. CM7 will
	 * acquire a specific HSEM to indicate that CM4 can proceed.
	 */
	if (!LL_RCC_IsCM4BootForced()) {
		while (!LL_HSEM_IsSemaphoreLocked(HSEM, CFG_HW_ENTRY_STOP_MODE_SEMID)) {
			/* Wait for CM7 to lock the HSEM after system initialization */
		}
	}
}
