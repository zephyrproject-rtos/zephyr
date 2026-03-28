/*
 * Copyright (c) 2019 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/led.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#define LED_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(holtek_ht16k33)

int main(void)
{
	const struct device *const led = DEVICE_DT_GET(LED_NODE);
	int i;

	if (!device_is_ready(led)) {
		LOG_ERR("LED device not ready");
		return 0;
	}

	while (1) {
		LOG_INF("Iterating through all LEDs, turning them on "
			"one-by-one");
		for (i = 0; i < 128; i++) {
			led_on(led, i);
			k_sleep(K_MSEC(100));
		}

		for (i = 500; i <= 2000; i *= 2) {
			LOG_INF("Blinking LEDs with a period of %d ms", i);
			led_blink(led, 0, i / 2, i / 2);
			k_msleep(10 * i);
		}
		led_blink(led, 0, 0, 0);

		for (i = 100; i >= 0; i -= 10) {
			LOG_INF("Setting LED brightness to %d%%", i);
			led_set_brightness(led, 0, i);
			k_sleep(K_MSEC(1000));
		}

		LOG_INF("Turning all LEDs off and restoring 100%% brightness");
		for (i = 0; i < 128; i++) {
			led_off(led, i);
		}
		led_set_brightness(led, 0, 100);
	}
	return 0;
}
