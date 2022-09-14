/*
 * Copyright (c) 2016 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32F3 processor
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <stm32_ll_system.h>

#include <cmsis_core.h>

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int stm32f3_init(void)
{
	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 8 MHz from HSI */
	SystemCoreClock = 8000000;

	/* Allow reflashing the board */
	LL_DBGMCU_EnableDBGSleepMode();

	return 0;
}

SYS_INIT(stm32f3_init, PRE_KERNEL_1, 0);
