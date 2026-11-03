/*
 * Copyright (c) 2022 Benjamin Björnsson <benjamin.bjornsson@gmail.com>.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_gpio.h>

void board_early_init_hook(void)
{
	/* HSE is an externally powered oscillator on the Portenta H7. Enable its
	 * supply before the STM32 clock driver waits for HSE ready. The regulator
	 * device is initialized at POST_KERNEL, after clock setup has already run.
	 */
	LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOH);

	/* Preload the output latch high to avoid a low pulse when changing mode. */
	LL_GPIO_SetOutputPin(GPIOH, LL_GPIO_PIN_1);
	LL_GPIO_SetPinMode(GPIOH, LL_GPIO_PIN_1, LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetPinSpeed(GPIOH, LL_GPIO_PIN_1, LL_GPIO_SPEED_FREQ_LOW);
	LL_GPIO_SetPinOutputType(GPIOH, LL_GPIO_PIN_1, LL_GPIO_OUTPUT_PUSHPULL);
	LL_GPIO_SetPinPull(GPIOH, LL_GPIO_PIN_1, LL_GPIO_PULL_NO);
}


void board_late_init_hook(void)
{

	/* Set led1 inactive since the Arduino bootloader leaves it active */
	const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

	if (!gpio_is_ready_dt(&led1)) {
		return;
	}

	(void)gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
}
