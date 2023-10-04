/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>

#define LED_R_PIN  DT_GPIO_PIN(DT_ALIAS(led2), gpios)
#define LED_G_PIN  DT_GPIO_PIN(DT_ALIAS(led1), gpios)
#define LED_B_PIN  DT_GPIO_PIN(DT_ALIAS(led0), gpios)
#define BL_PIN     5

static int board_esp_wrover_kit_init(void)
{
	const struct device *gpio;

	gpio = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	if (!device_is_ready(gpio)) {
		return -ENODEV;
	}

	/* turns red LED off */
	gpio_pin_configure(gpio, LED_R_PIN, GPIO_OUTPUT);
	gpio_pin_set(gpio, LED_R_PIN, 0);

	/* turns green LED off */
	gpio_pin_configure(gpio, LED_G_PIN, GPIO_OUTPUT);
	gpio_pin_set(gpio, LED_G_PIN, 0);

	/* turns blue LED off */
	gpio_pin_configure(gpio, LED_B_PIN, GPIO_OUTPUT);
	gpio_pin_set(gpio, LED_B_PIN, 0);

	/* turns LCD backlight on */
	gpio_pin_configure(gpio, BL_PIN, GPIO_OUTPUT);
	gpio_pin_set(gpio, BL_PIN, 0);

	return 0;
}

SYS_INIT(board_esp_wrover_kit_init, APPLICATION, CONFIG_GPIO_INIT_PRIORITY);
