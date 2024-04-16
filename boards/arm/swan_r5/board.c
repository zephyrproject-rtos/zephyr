/*
 * Copyright (c) 2022 Blues Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>

static int board_swan_init(void)
{
	const struct gpio_dt_spec dischrg =
		GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), dischrg_gpios);


	if (!gpio_is_ready_dt(&dischrg)) {
		return -ENODEV;
	}

	(void)gpio_pin_configure_dt(&dischrg, GPIO_OUTPUT_INACTIVE);

	return 0;
}

SYS_INIT(board_swan_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
