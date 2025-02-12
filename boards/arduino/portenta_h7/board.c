/*
 * Copyright (c) 2022 Benjamin Björnsson <benjamin.bjornsson@gmail.com>.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>

static int board_init(void)
{

	/* Set led1 inactive since the Arduino bootloader leaves it active */
	const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

	if (!gpio_is_ready_dt(&led1)) {
		return -ENODEV;
	}

	return gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
}

SYS_INIT(board_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
