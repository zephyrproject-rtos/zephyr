/*
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32L4 processor
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <stm32_ll_bus.h>

#include <cmsis_core.h>

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int stm32m4_init(void)
{
	/*HW semaphore Clock enable*/
	LL_AHB3_GRP1_EnableClock(LL_AHB3_GRP1_PERIPH_HSEM);

	/* Update CMSIS SystemCoreClock variable (HCLK) */
	SystemCoreClock = 209000000;

	return 0;
}

SYS_INIT(stm32m4_init, PRE_KERNEL_1, 0);
