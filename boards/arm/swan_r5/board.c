/*
 * Copyright (c) 2022 Blues Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>

static int board_swan_init(const struct device *dev)
{
	const struct gpio_dt_spec gpio6 =
		GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), no_pull_gpios);

	ARG_UNUSED(dev);

	if (!device_is_ready(gpio6.port)) {
		return -ENODEV;
	}

	(void)gpio_pin_configure_dt(&gpio6, (GPIO_NOPULL | GPIO_MODE_ANALOG));

	return 0;
}

SYS_INIT(board_swan_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
