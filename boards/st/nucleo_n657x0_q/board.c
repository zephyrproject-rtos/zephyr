/*
 * Copyright (c) 2026 Lubor Witkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/platform/hooks.h>

#include <stm32_ll_bus.h>
#include <stm32_ll_gpio.h>

/*
 * VDDCORE comes from an external switched-mode power supply (SMPS) whose output
 * is selected by PB12 (EXT_SMPS_MODE/PWR_LP, see MB1940 schematic):
 * PWR_LP = 1 (OVERDRIVE): VDDCORE = 0.89 V
 * PWR_LP = 0 (NOMINAL): VDDCORE = 0.81 V
 *
 * The SoC init selects VOS0, and the board clock tree runs the CPU at 800 MHz,
 * and the NPU at 1 GHz, which need the overdrive voltage, so raise it before
 * the clocks are configured.
 */
void board_early_init_hook(void)
{
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOB);

	LL_GPIO_SetPinOutputType(GPIOB, LL_GPIO_PIN_12, LL_GPIO_OUTPUT_PUSHPULL);
	LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_12);
	LL_GPIO_SetPinPull(GPIOB, LL_GPIO_PIN_12, LL_GPIO_PULL_NO);
	LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_12, LL_GPIO_MODE_OUTPUT);
}
