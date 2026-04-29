/*
 * Copyright (c) 2026 Hubert Miś
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <errno.h>
#include <stdint.h>
#include <zephyr/drivers/led.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#define LED_NODE_ID	 DT_NODELABEL(leds)
#define LED_NUM_CHANNELS	DT_CHILD_NUM(LED_NODE_ID)

int main(void)
{
	const struct device *leds;
	uint8_t all_off[LED_NUM_CHANNELS];
	uint8_t all_on[LED_NUM_CHANNELS];
	int ret;

	leds = DEVICE_DT_GET(LED_NODE_ID);
	if (!device_is_ready(leds)) {
		LOG_ERR("Device %s is not ready", leds->name);
		return 0;
	}

	memset(all_off, 0, ARRAY_SIZE(all_off));
	memset(all_on, 100, ARRAY_SIZE(all_on));

	while (1) {
		LOG_INF("Turning on sequence");
		for (int i = 0; i < LED_NUM_CHANNELS; i++) {
			led_on(leds, i);
			k_sleep(K_MSEC(250));
		}
		LOG_INF("Turned on");

		ret = led_write_channels(leds, 0, LED_NUM_CHANNELS, all_off);

		if (ret == -ENOSYS) {
			LOG_INF("led_write_channels is not available");
			k_sleep(K_SECONDS(1));
		} else if (ret < 0) {
			LOG_ERR("led_write_channels returned %d", ret);
		} else {
			LOG_INF("Blinking with led_write_channels");

			k_sleep(K_MSEC(250));
			ret = led_write_channels(leds, 0, LED_NUM_CHANNELS, all_on);

			if (ret < 0) {
				LOG_ERR("led_write_channels returned %d", ret);
			}

			k_sleep(K_MSEC(250));
			ret = led_write_channels(leds, 0, LED_NUM_CHANNELS, all_off);

			if (ret < 0) {
				LOG_ERR("led_write_channels returned %d", ret);
			}

			k_sleep(K_MSEC(250));
			ret = led_write_channels(leds, 0, LED_NUM_CHANNELS, all_on);

			if (ret < 0) {
				LOG_ERR("led_write_channels returned %d", ret);
			}

			k_sleep(K_MSEC(250));
		}

		LOG_INF("Turning off sequence");
		for (int i = 0; i < LED_NUM_CHANNELS; i++) {
			led_off(leds, i);
			k_sleep(K_MSEC(250));
		}
		LOG_INF("Turned off");
	}

	return 0;
}
