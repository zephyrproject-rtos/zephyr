/*
 * Copyright (c) 2018 Savoir-Faire Linux.
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
LOG_MODULE_REGISTER(main);

#define LED_DEV_NAME DT_LABEL(DT_INST(0, nxp_pca9633))
#define NUM_LEDS 4
#define MAX_BRIGHTNESS 100
#define HALF_BRIGHTNESS (MAX_BRIGHTNESS / 2)
#define BLINK_DELAY_ON 500
#define BLINK_DELAY_OFF 500
#define DELAY_TIME_MS 1000
#define DELAY_TIME K_MSEC(DELAY_TIME_MS)

void main(void)
{
	const struct device *led_dev;
	int i, ret;

	led_dev = device_get_binding(LED_DEV_NAME);
	if (led_dev) {
		LOG_INF("Found LED device %s", LED_DEV_NAME);
	} else {
		LOG_ERR("LED device %s not found", LED_DEV_NAME);
		return;
	}

	LOG_INF("Testing leds");

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
		for (i = 0; i < NUM_LEDS; i++) {
			ret = led_off(led_dev, i);
			if (ret < 0) {
				return;
			}

			k_sleep(DELAY_TIME);
		}

		/* Set the brightness to half max of LEDs one by one */
		for (i = 0; i < NUM_LEDS; i++) {
			ret = led_set_brightness(led_dev, i, HALF_BRIGHTNESS);
			if (ret < 0) {
				return;
			}

			k_sleep(DELAY_TIME);
		}

		/* Turn off LEDs one by one */
		for (i = 0; i < NUM_LEDS; i++) {
			ret = led_off(led_dev, i);
			if (ret < 0) {
				return;
			}

			k_sleep(DELAY_TIME);
		}

		/* Test the blinking of LEDs one by one */
		for (i = 0; i < NUM_LEDS; i++) {
			ret = led_blink(led_dev, i, BLINK_DELAY_ON,
					BLINK_DELAY_OFF);
			if (ret < 0) {
				return;
			}

			k_sleep(DELAY_TIME);
		}

		/* Wait a few blinking before turning off the LEDs */
		k_msleep(DELAY_TIME_MS * 10);

		/* Turn off LEDs one by one */
		for (i = 0; i < NUM_LEDS; i++) {
			ret = led_off(led_dev, i);
			if (ret < 0) {
				return;
			}

			k_sleep(DELAY_TIME);
		}

	}
}
