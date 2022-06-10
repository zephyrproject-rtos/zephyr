/*
 * Copyright (c) 2022 Yannis Damigos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>

#define LED_B_PIN  DT_GPIO_PIN(DT_ALIAS(led0), gpios)

static int board_odroid_go_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	const struct device *gpio;

	gpio = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	if (!device_is_ready(gpio)) {
		return -ENODEV;
	}

	/* turns blue LED off */
	gpio_pin_configure(gpio, LED_B_PIN, GPIO_OUTPUT);
	gpio_pin_set(gpio, LED_B_PIN, 0);

	return 0;
}

SYS_INIT(board_odroid_go_init, APPLICATION, CONFIG_GPIO_INIT_PRIORITY);
