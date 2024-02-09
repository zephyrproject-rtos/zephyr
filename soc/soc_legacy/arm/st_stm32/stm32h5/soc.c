/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32H5 processor
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_icache.h>
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
static int stm32h5_init(void)
{
	/* Enable instruction cache in 1-way (direct mapped cache) */
	LL_ICACHE_SetMode(LL_ICACHE_1WAY);
	LL_ICACHE_Enable();

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 32 MHz from HSI with a HSIDIV = 2 */
	SystemCoreClock = 32000000;

#if defined(PWR_UCPDR_UCPD_DBDIS)
	/* Disable USB Type-C dead battery pull-down behavior */
	LL_PWR_DisableUCPDDeadBattery();
#endif /* PWR_UCPDR_UCPD_DBDIS */

	return 0;
}

SYS_INIT(stm32h5_init, PRE_KERNEL_1, 0);
