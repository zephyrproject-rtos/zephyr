/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

/* 1000 msec = 1 sec */
#define SLEEP_SHORT_TIME_MS   500
#define SLEEP_LONG_TIME_MS   1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
#define LED0	DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN     DT_GPIO_PIN(LED0_NODE, gpios)
#define FLAGS	DT_GPIO_FLAGS(LED0_NODE, gpios)
#else
/* A build error here means your board isn't set up to blink an LED. */
#error "Unsupported board: led0 devicetree alias is not defined"
#define LED0	""
#define PIN     0
#define FLAGS	0
#endif

void main(void)
{
	const struct device *dev;
	bool led_is_on = false;
	int ret;

	dev = device_get_binding(LED0);
	if (dev == NULL) {
		return;
	}

	ret = gpio_pin_configure(dev, PIN, GPIO_OUTPUT_ACTIVE | FLAGS);
	if (ret < 0) {
		return;
	}

	/*
	 * Many boards ship with a basic "blinky" pattern running already.
	 * In order to be able to distinguish the Zephyr blinky, it has a
	 * rather unique pattern.
	 *
	 * LED pattern is:
	 *     on:      short short     short      long
	 *     off:        short   long      long        short
	 *
	 * As a waveform:
	 *     on:      +--+  +--+      +--+      +------+  + ...
	 *     off: ... +  +--+  +------+  +------+      +--+
	 */
	while (1) {
		led_is_on = true;
		gpio_pin_set(dev, PIN, (int)led_is_on);
		k_msleep(SLEEP_SHORT_TIME_MS);

		led_is_on = false;
		gpio_pin_set(dev, PIN, (int)led_is_on);
		k_msleep(SLEEP_SHORT_TIME_MS);

		led_is_on = true;
		gpio_pin_set(dev, PIN, (int)led_is_on);
		k_msleep(SLEEP_SHORT_TIME_MS);

		led_is_on = false;
		gpio_pin_set(dev, PIN, (int)led_is_on);
		k_msleep(SLEEP_LONG_TIME_MS);

		led_is_on = true;
		gpio_pin_set(dev, PIN, (int)led_is_on);
		k_msleep(SLEEP_SHORT_TIME_MS);

		led_is_on = false;
		gpio_pin_set(dev, PIN, (int)led_is_on);
		k_msleep(SLEEP_LONG_TIME_MS);

		led_is_on = true;
		gpio_pin_set(dev, PIN, (int)led_is_on);
		k_msleep(SLEEP_LONG_TIME_MS);

		led_is_on = false;
		gpio_pin_set(dev, PIN, (int)led_is_on);
		k_msleep(SLEEP_SHORT_TIME_MS);
	}
}
