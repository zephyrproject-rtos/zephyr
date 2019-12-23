/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <errno.h>
#include <drivers/led.h>
#include <sys/util.h>
#include <sys/printk.h>
#include <zephyr.h>

#define LED_DEV_NAME "LED_GPIO_0"
#define NUM_LEDS 2
#define BLINK_DELAY_ON 500
#define BLINK_DELAY_OFF 500
#define DELAY_TIME 1000

static uint8_t led_id[NUM_LEDS] = {DT_PROP(DT_NODELABEL(led0), id),
				   DT_PROP(DT_NODELABEL(led1), id)};

void main(void)
{
	struct device *led_dev;
	int i, ret;

	led_dev = device_get_binding(LED_DEV_NAME);
	if (led_dev) {
		printk("Found LED device %s\n", LED_DEV_NAME);
	} else {
		printk("LED device %s not found\n", LED_DEV_NAME);
		return;
	}

	printk("Testing leds");

	while (1) {
		/* Turn on LEDs one by one */
		for (i = 0; i < NUM_LEDS; i++) {
			ret = led_on(led_dev, led_id[i]);
			if (ret < 0) {
				return;
			}

			k_msleep(DELAY_TIME);
		}

		/* Turn off LEDs one by one */
		for (i = 0; i < NUM_LEDS; i++) {
			ret = led_off(led_dev, led_id[i]);
			if (ret < 0) {
				return;
			}

			k_msleep(DELAY_TIME);
		}

		/* Test the blinking of LEDs one by one */
		for (i = 0; i < NUM_LEDS; i++) {
			ret = led_blink(led_dev, led_id[i], BLINK_DELAY_ON,
					BLINK_DELAY_OFF);
			if (ret < 0) {
				return;
			}

			k_msleep(DELAY_TIME);
		}

		/* Wait a few blinking before turning off the LEDs */
		k_msleep(DELAY_TIME * 10);

		/* Turn off LEDs one by one */
		for (i = 0; i < NUM_LEDS; i++) {
			ret = led_off(led_dev, led_id[i]);
			if (ret < 0) {
				return;
			}

			k_msleep(DELAY_TIME);
		}
	}
}
