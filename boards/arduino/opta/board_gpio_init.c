/*
 * Copyright (c) 2024 DNDG srl
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <stm32h7xx_ll_bus.h>
#include <stm32h7xx_ll_gpio.h>

static int board_gpio_init(void)
{
	/* The external oscillator that drives the HSE clock should be enabled
	 * by setting the GPIOI1 pin. This function is registered at priority
	 * RE_KERNEL_1 to be executed before the standard STM clock
	 * setup code.
	 *
	 * Note that the HSE should be turned on by the M7 only because M4
	 * is not booted by default on Opta and cannot configure the clocks
	 * anyway.
	 */
#ifdef CONFIG_BOARD_ARDUINO_OPTA_STM32H747XX_M7
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOH);
	LL_GPIO_SetPinMode(GPIOH, LL_GPIO_PIN_1, LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetPinSpeed(GPIOH, LL_GPIO_PIN_1, LL_GPIO_SPEED_FREQ_LOW);
	LL_GPIO_SetPinOutputType(GPIOH, LL_GPIO_PIN_1, LL_GPIO_OUTPUT_PUSHPULL);
	LL_GPIO_SetPinPull(GPIOH, LL_GPIO_PIN_1, LL_GPIO_PULL_UP);
	LL_GPIO_SetOutputPin(GPIOH, LL_GPIO_PIN_1);
#endif

	/* The ethernet adapter is enabled by settig the GPIOJ15 pin to 1.
	 * This is done only if the network has been explicitly configured
	 */
#ifdef CONFIG_NET_L2_ETHERNET
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOJ);
	LL_GPIO_SetPinMode(GPIOJ, LL_GPIO_PIN_15, LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetPinSpeed(GPIOJ, LL_GPIO_PIN_15, LL_GPIO_SPEED_FREQ_LOW);
	LL_GPIO_SetPinOutputType(GPIOJ, LL_GPIO_PIN_15, LL_GPIO_OUTPUT_PUSHPULL);
	LL_GPIO_SetPinPull(GPIOJ, LL_GPIO_PIN_15, LL_GPIO_PULL_UP);
	LL_GPIO_SetOutputPin(GPIOJ, LL_GPIO_PIN_15);
#endif

	return 0;
}

SYS_INIT(board_gpio_init, PRE_KERNEL_1, 0);
