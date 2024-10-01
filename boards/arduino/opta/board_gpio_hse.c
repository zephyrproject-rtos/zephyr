/*
 * Copyright (c) 2024 DNDG srl
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <stm32h7xx_ll_bus.h>
#include <stm32h7xx_ll_gpio.h>

static int board_gpio_hse(void)
{
	/* The external oscillator that drives the HSE clock should be enabled
	 * by setting the GPIOI1 pin. This function is registered at priority
	 * RE_KERNEL_1 to be executed before the standard STM clock
	 * setup code.
	 */

	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOH);

	LL_GPIO_SetPinMode(GPIOH, LL_GPIO_PIN_1, LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetPinSpeed(GPIOH, LL_GPIO_PIN_1, LL_GPIO_SPEED_FREQ_LOW);
	LL_GPIO_SetPinOutputType(GPIOH, LL_GPIO_PIN_1, LL_GPIO_OUTPUT_PUSHPULL);
	LL_GPIO_SetPinPull(GPIOH, LL_GPIO_PIN_1, LL_GPIO_PULL_UP);
	LL_GPIO_SetOutputPin(GPIOH, LL_GPIO_PIN_1);

	return 0;
}

SYS_INIT(board_gpio_hse, PRE_KERNEL_1, 0);
