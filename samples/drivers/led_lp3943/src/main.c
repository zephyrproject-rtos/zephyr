/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <errno.h>
#include <drivers/led.h>
#include <sys/util.h>
#include <zephyr.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(app);

#define LED_DEV_NAME DT_INST_0_TI_LP3943_LABEL
#define NUM_LEDS 16

#define DELAY_TIME K_MSEC(1000)

void main(void)
{
	struct device *led_dev;
	int i, ret;

	led_dev = device_get_binding(LED_DEV_NAME);
	if (led_dev) {
		LOG_INF("Found LED device %s", LED_DEV_NAME);
	} else {
		LOG_ERR("LED device %s not found", LED_DEV_NAME);
		return;
	}

	/*
	 * Display a continuous pattern that turns on 16 LEDs at 1s one by
	 * one until it reaches the end and turns off LEDs in reverse order.
	 */

	LOG_INF("Displaying the pattern");

	while (1) {
		/* Turn on LEDs one by one */
		for (i = 0; i < NUM_LEDS; i++) {
			ret = led_on(led_dev, i);
			if (ret < 0) {
				return;
			}

			k_sleep(DELAY_TIME);
		}

		/* Turn off LEDs one by one */
		for (i = NUM_LEDS - 1; i >= 0; i--) {
			ret = led_off(led_dev, i);
			if (ret < 0) {
				return;
			}

			k_sleep(DELAY_TIME);
		}
	}
}
