/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

#ifndef DT_ALIAS_LED0_GPIOS_FLAGS
#define FLAGS 0
#else
#define FLAGS DT_ALIAS_LED0_GPIOS_FLAGS
#endif

/* Make sure the board's devicetree declares led0 in its /aliases. */
#ifdef DT_ALIAS_LED0_GPIOS_CONTROLLER
#define LED0	DT_ALIAS_LED0_GPIOS_CONTROLLER
#define PIN	DT_ALIAS_LED0_GPIOS_PIN
#else
#error "Unsupported board: led0 devicetree alias is not defined"
#define LED0	""
#define PIN	0
#endif

void main(void)
{
	struct device *dev;
	bool led_is_on = true;
	int ret;

	dev = device_get_binding(LED0);
	if (dev == NULL) {
		return;
	}

	ret = gpio_pin_configure(dev, PIN, GPIO_OUTPUT_ACTIVE | FLAGS);
	if (ret < 0) {
		return;
	}

	while (1) {
		gpio_pin_set(dev, PIN, (int)led_is_on);
		led_is_on = !led_is_on;
		k_sleep(SLEEP_TIME_MS);
	}
}
