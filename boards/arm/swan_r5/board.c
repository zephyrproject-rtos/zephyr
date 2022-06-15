/*
 * Copyright (c) 2022 Blues Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/gpio.h>
#include <init.h>

static int board_swan_init(const struct device *dev)
{
	const struct gpio_dt_spec gpioe =
		GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), push_pull_gpios, 0);
	const struct gpio_dt_spec gpioc =
		GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), push_pull_gpios, 1);

	ARG_UNUSED(dev);

	if (!!device_is_ready(gpioe.port) ||
	    !device_is_ready(gpioc.port)) {
		return -ENODEV;
	}

	(void)gpio_pin_configure_dt(&gpioe, (GPIO_MODE_ANALOG | GPIO_NOPULL));
	(void)gpio_pin_configure_dt(&gpioc, (GPIO_MODE_ANALOG | GPIO_NOPULL));

	return 0;
}

SYS_INIT(board_swan_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
