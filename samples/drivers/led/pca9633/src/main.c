/*
 * Copyright (c) 2018 Savoir-Faire Linux.
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
LOG_MODULE_REGISTER(main);

#define NUM_LEDS 4
#define MAX_BRIGHTNESS 100
#define HALF_BRIGHTNESS (MAX_BRIGHTNESS / 2)
#define BLINK_DELAY_ON 500
#define BLINK_DELAY_OFF 500
#define DELAY_TIME_MS 1000
#define DELAY_TIME K_MSEC(DELAY_TIME_MS)

int main(void)
{
	const struct device *const led_dev = DEVICE_DT_GET_ANY(nxp_pca9633);
	int i, ret;

	if (!led_dev) {
		LOG_ERR("No devices with compatible nxp,pca9633 found");
		return 0;
	} else if (!device_is_ready(led_dev)) {
		LOG_ERR("LED device %s is not ready", led_dev->name);
		return 0;
	} else {
		LOG_INF("Found LED device %s", led_dev->name);
	}

	LOG_INF("Testing leds");

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
		for (i = 0; i < NUM_LEDS; i++) {
			ret = led_off(led_dev, i);
			if (ret < 0) {
				return 0;
			}

			k_sleep(DELAY_TIME);
		}

		/* Set the brightness to half max of LEDs one by one */
		for (i = 0; i < NUM_LEDS; i++) {
			ret = led_set_brightness(led_dev, i, HALF_BRIGHTNESS);
			if (ret < 0) {
				return 0;
			}

			k_sleep(DELAY_TIME);
		}

		/* Turn off LEDs one by one */
		for (i = 0; i < NUM_LEDS; i++) {
			ret = led_off(led_dev, i);
			if (ret < 0) {
				return 0;
			}

			k_sleep(DELAY_TIME);
		}

		/* Test the blinking of LEDs one by one */
		for (i = 0; i < NUM_LEDS; i++) {
			ret = led_blink(led_dev, i, BLINK_DELAY_ON,
					BLINK_DELAY_OFF);
			if (ret < 0) {
				return 0;
			}

			k_sleep(DELAY_TIME);
		}

		/* Wait a few blinking before turning off the LEDs */
		k_msleep(DELAY_TIME_MS * 10);

		/* Turn off LEDs one by one */
		for (i = 0; i < NUM_LEDS; i++) {
			ret = led_off(led_dev, i);
			if (ret < 0) {
				return 0;
			}

			k_sleep(DELAY_TIME);
		}

	}
	return 0;
}
