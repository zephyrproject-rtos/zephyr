/*
 * Copyright (c) 2023 Benjamin Björnsson <benjamin.bjornsson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for STM32C0 processor
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/linker/linker-defs.h>
#include <string.h>

#include <cmsis_core.h>

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int stm32c0_init(void)
{
	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 48 MHz from HSI */
	SystemCoreClock = 48000000;

	return 0;
}

SYS_INIT(stm32c0_init, PRE_KERNEL_1, 0);
