/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief STM32MP25 Cortex-M33 initialization code
 */

#include <soc.h>

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 */
void soc_early_init_hook(void)
{
	/* Update CMSIS SystemCoreClock variable (HCLK) */
	SystemCoreClockUpdate();
}
