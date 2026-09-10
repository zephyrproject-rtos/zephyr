/*
 * Copyright (c) 2023 The ChromiumOS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>

#include "meas.h"

enum led_color_t {
	LED_RED,
	LED_GREEN,
	LED_BLUE,
};

static void set_led(const struct device *const led, enum led_color_t led_color)
{
	if (led_color != LED_RED) {
		led_off(led, LED_RED);
	}
	if (led_color != LED_GREEN) {
		led_off(led, LED_GREEN);
	}
	if (led_color != LED_BLUE) {
		led_off(led, LED_BLUE);
	}
	led_on(led, led_color);
}

#define CHARGING_VOLTAGE 5000
#define CHARGING_CURRENT 1000

int main(void)
{
	meas_init();

	const struct device *const led = DEVICE_DT_GET_ONE(gpio_leds);
	int32_t vbus_v = 0;
	int32_t vbus_c = 0;

	if (!device_is_ready(led)) {
		return 0;
	}

	while (1) {
		meas_vbus_v(&vbus_v);
		meas_vbus_c(&vbus_c);

		if (vbus_v > CHARGING_VOLTAGE) {
			if (vbus_c > CHARGING_CURRENT) {
				set_led(led, LED_GREEN);
			} else {
				set_led(led, LED_BLUE);
			}
		} else {
			set_led(led, LED_RED);
		}

		k_usleep(500);
	}
}
