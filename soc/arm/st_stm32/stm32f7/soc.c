/*
 * Copyright (c) 2018 Yurii Hamann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32F7 processor
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>

#include <cmsis_core.h>
#include <stm32_ll_system.h>

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int st_stm32f7_init(void)
{
	/* Enable ART Flash cache accelerator */
	LL_FLASH_EnableART();

	SCB_EnableICache();

	if (IS_ENABLED(CONFIG_DCACHE)) {
		if (!(SCB->CCR & SCB_CCR_DC_Msk)) {
			SCB_EnableDCache();
		}
	}

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 16 MHz from HSI */
	SystemCoreClock = 16000000;

	return 0;
}

SYS_INIT(st_stm32f7_init, PRE_KERNEL_1, 0);
