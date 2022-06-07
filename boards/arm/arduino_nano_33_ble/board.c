/*
 * Copyright (c) 2020 Jefferson Lee.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>

static int board_init(const struct device *dev)
{
	const struct gpio_dt_spec pull_up =
		GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), pull_up_gpios);

	ARG_UNUSED(dev);

	if (!device_is_ready(pull_up.port)) {
		return -ENODEV;
	}

	return gpio_pin_configure_dt(&pull_up, GPIO_OUTPUT_HIGH);
}

SYS_INIT(board_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
