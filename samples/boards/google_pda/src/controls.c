/*
 * Copyright (c) 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* The devicetree node identifier for the cc enable pins. */
#define ENCC1_NODE DT_ALIAS(encc1)
#define ENCC2_NODE DT_ALIAS(encc2)

static const struct gpio_dt_spec encc1 = GPIO_DT_SPEC_GET(ENCC1_NODE, gpios);
static const struct gpio_dt_spec encc2 = GPIO_DT_SPEC_GET(ENCC2_NODE, gpios);

void en_cc1(bool en) {
	gpio_pin_set_dt(&encc1, en);
}

void en_cc2(bool en) {
	gpio_pin_set_dt(&encc2, en);
}

int controls_init(void)
{
	int ret;

	if (!device_is_ready(encc1.port)) {
		return -EIO;
	}

	if (!device_is_ready(encc2.port)) {
		return -EIO;
	}

	ret = gpio_pin_configure_dt(&encc1, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&encc2, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	return 0;
}
