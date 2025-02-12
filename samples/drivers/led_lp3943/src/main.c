/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <errno.h>
#include <zephyr/drivers/led.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

#define NUM_LEDS 16

#define DELAY_TIME K_MSEC(1000)

int main(void)
{
	const struct device *const led_dev = DEVICE_DT_GET_ANY(ti_lp3943);
	int i, ret;

	if (!led_dev) {
		LOG_ERR("No device with compatible ti,lp3943 found");
		return 0;
	} else if (!device_is_ready(led_dev)) {
		LOG_ERR("LED device %s not ready", led_dev->name);
		return 0;
	} else {
		LOG_INF("Found LED device %s", led_dev->name);
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
				return 0;
			}

			k_sleep(DELAY_TIME);
		}

		/* Turn off LEDs one by one */
		for (i = NUM_LEDS - 1; i >= 0; i--) {
			ret = led_off(led_dev, i);
			if (ret < 0) {
				return 0;
			}

			k_sleep(DELAY_TIME);
		}
	}
	return 0;
}
