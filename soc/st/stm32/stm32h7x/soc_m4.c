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

	/* In case CM4 has not been forced boot by CM7,
	 * CM4 needs to wait until CM7 has setup clock configuration
	 */
	if (!LL_RCC_IsCM4BootForced()) {
		/*
		 * Domain D2 is waiting for Cortex-M7 to perform
		 * system initialization
		 * (system clock config, external memory configuration.. ).
		 * End of system initialization is reached when CM7 takes HSEM.
		 */
		while ((HSEM->RLR[CFG_HW_ENTRY_STOP_MODE_SEMID] & HSEM_R_LOCK)
				!= HSEM_R_LOCK) {
			;
		}
	}
}
