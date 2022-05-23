/*
 * Copyright (c) 2022 Benjamin Björnsson <benjamin.bjornsson@gmail.com>.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/zephyr.h>
#include <zephyr/drivers/led.h>
#include <zephyr/device.h>

#define LED_NUM_MAX 3
#define LED_BRIGHTNESS_MAX 100

int led_fade_in(const struct device *led_dev, uint32_t led, uint32_t duration_ms)
{
	int res;

	for (int i = 0; i <= LED_BRIGHTNESS_MAX; i++) {
		res = led_set_brightness(led_dev, led, i);
		if (res) {
			LOG_ERR("Failed to turn on led: %d", res);
			return res;
		}

		k_msleep(duration_ms / LED_BRIGHTNESS_MAX);
	}

	return 0;
}

int led_fade_out(const struct device *led_dev, uint32_t led, uint32_t duration_ms)
{
	int res;

	for (int i = LED_BRIGHTNESS_MAX; i >= 0; i--) {
		res = led_set_brightness(led_dev, led, i);
		if (res) {
			LOG_ERR("Failed to turn on led: %d", res);
			return res;
		}

		k_msleep(duration_ms / LED_BRIGHTNESS_MAX);
	}

	return 0;
}

int led_fade_in_out(const struct device *led_dev, uint32_t led, uint32_t duration_in_ms,
		    uint32_t duration_hold_ms, uint32_t duration_out_ms)
{
	int res;

	res = led_fade_in(led_dev, led, duration_in_ms);
	if (res) {
		LOG_ERR("Failed to turn on led: %d", res);
		return res;
	}

	res = led_fade_out(led_dev, led, duration_out_ms);
	if (res) {
		LOG_ERR("Failed to turn on led: %d", res);
		return res;
	}

	return 0;
}

void main(void)
{
	const struct device *led_dev = DEVICE_DT_GET(DT_NODELABEL(is31fl3194));
	const struct led_info *leds_info[LED_NUM_MAX];
	int res;

	if (!device_is_ready(led_dev)) {
		LOG_ERR("LED device %s not ready", led_dev->name);
		return;
	}

	for (int i = 0; i < LED_NUM_MAX; i++) {
		res = led_get_info(led_dev, i, &leds_info[i]);
		if (res) {
			LOG_INF("No led info at led: %d", i);
		}

		LOG_INF("Running fade in and fade out with led: %s",
			(leds_info[i] != NULL) ? leds_info[i]->label : "Unknown");
		res = led_fade_in_out(led_dev, i, 2000, 500, 1000);
		if (res) {
			LOG_ERR("Failed to fade led %d in and out", res);
			return;
		}
	}
}
