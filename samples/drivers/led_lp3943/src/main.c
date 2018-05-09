/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <errno.h>
#include <led.h>
#include <misc/util.h>
#include <zephyr.h>

#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#include <logging/sys_log.h>

#define LED_DEV_NAME CONFIG_LP3943_DEV_NAME
#define NUM_LEDS 16

#define DELAY_TIME K_MSEC(1000)

void main(void)
{
	struct device *led_dev;
	int i, ret;

	led_dev = device_get_binding(LED_DEV_NAME);
	if (led_dev) {
		SYS_LOG_INF("Found LED device %s", LED_DEV_NAME);
	} else {
		SYS_LOG_ERR("LED device %s not found", LED_DEV_NAME);
		return;
	}

	/*
	 * Display a continuous pattern that turns on 16 LEDs at 1s one by
	 * one until it reaches the end and turns off LEDs in reverse order.
	 */

	SYS_LOG_INF("Displaying the pattern");

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
