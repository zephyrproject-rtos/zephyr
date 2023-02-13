/*
 * Copyright (c) 2022 Henrik Brix Andersen <henrik@brixandersen.dk>
 * Copyright 2018 Foundries.io Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>

static int rv32m1_vega_board_init(const struct device *dev)
{
	const struct device *const gpiob = DEVICE_DT_GET(DT_NODELABEL(gpiob));
	const struct device *const gpioc = DEVICE_DT_GET(DT_NODELABEL(gpioc));
	const struct device *const gpiod = DEVICE_DT_GET(DT_NODELABEL(gpiod));

	ARG_UNUSED(dev);

	__ASSERT_NO_MSG(device_is_ready(gpiob));
	__ASSERT_NO_MSG(device_is_ready(gpioc));
	__ASSERT_NO_MSG(device_is_ready(gpiod));

	gpio_pin_configure(gpiob, 29, GPIO_OUTPUT);

	gpio_pin_configure(gpioc, 28, GPIO_OUTPUT);
	gpio_pin_configure(gpioc, 29, GPIO_OUTPUT);
	gpio_pin_configure(gpioc, 30, GPIO_OUTPUT);

	gpio_pin_configure(gpiod, 0, GPIO_OUTPUT);
	gpio_pin_configure(gpiod, 1, GPIO_OUTPUT);
	gpio_pin_configure(gpiod, 2, GPIO_OUTPUT);
	gpio_pin_configure(gpiod, 3, GPIO_OUTPUT);
	gpio_pin_configure(gpiod, 4, GPIO_OUTPUT);
	gpio_pin_configure(gpiod, 5, GPIO_OUTPUT);

	return 0;
}

SYS_INIT(rv32m1_vega_board_init, PRE_KERNEL_1, CONFIG_BOARD_INIT_PRIORITY);
