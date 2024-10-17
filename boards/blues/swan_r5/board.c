/*
 * Copyright (c) 2022 Blues Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>

void board_late_init_hook(void)
{
	const struct gpio_dt_spec dischrg =
		GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), dischrg_gpios);


	if (!gpio_is_ready_dt(&dischrg)) {
		return -ENODEV;
	}

	(void)gpio_pin_configure_dt(&dischrg, GPIO_OUTPUT_INACTIVE);
}
