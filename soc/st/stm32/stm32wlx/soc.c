/*
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32WL processor
 */

#include <zephyr/device.h>
#include <zephyr/init.h>

#include <stm32wlxx_ll_system.h>

#include <zephyr/logging/log.h>

#include <cmsis_core.h>

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);


/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int stm32wl_init(void)
{
	/* Enable CPU data and instruction cache and prefetch */
	LL_FLASH_EnableInstCache();
	LL_FLASH_EnableDataCache();
	LL_FLASH_EnablePrefetch();

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 4 MHz from MSI */
	SystemCoreClock = 4000000;

	return 0;
}

SYS_INIT(stm32wl_init, PRE_KERNEL_1, 0);
