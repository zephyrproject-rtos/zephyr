/*
 * Copyright (c) 2021 Argentum Systems Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>

static int radio_off_setup(void)
{
	int ret;
	const struct gpio_dt_spec reset = GPIO_DT_SPEC_GET(DT_NODELABEL(lora), reset_gpios);
	const struct gpio_dt_spec cs = GPIO_DT_SPEC_GET(DT_NODELABEL(sercom4), cs_gpios);

	if (!gpio_is_ready_dt(&reset) || !gpio_is_ready_dt(&cs)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&reset, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&cs, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

SYS_INIT(radio_off_setup, PRE_KERNEL_1, 99);
