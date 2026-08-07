/*
 * Copyright (c) 2025 Hubert Miś
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#define LED_DELAY_MS    500
#define ON_OFF_DELAY_MS 1000

#define NUM_LEDS 16

int main(void)
{
	static const struct device *dev = DEVICE_DT_GET_ANY(sct_sct2024);

	if (!dev) {
		LOG_ERR("No SCT2024 LED controller found");
		return -ENODEV;
	}

	if (!device_is_ready(dev)) {
		LOG_ERR("SCT2024 LED controller not ready");
		return -ENODEV;
	}

	do {
		for (int i = 0; i < NUM_LEDS; i++) {
			if (led_on(dev, i)) {
				LOG_ERR("Failed to turn on LED %d", i);
			}
			k_msleep(LED_DELAY_MS);
		}

		k_msleep(ON_OFF_DELAY_MS);

		for (int i = 0; i < NUM_LEDS; i++) {
			if (led_off(dev, i)) {
				LOG_ERR("Failed to turn off LED %d", i);
			}
			k_msleep(LED_DELAY_MS);
		}

		k_msleep(ON_OFF_DELAY_MS);
	} while (true);

	return 0;
}
