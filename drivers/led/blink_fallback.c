/*
 * Copyright (c) 2025 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/led.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#define LED_ON  ((void *)1)
#define LED_OFF ((void *)0)

struct led_blink_fallback_data {
	const struct device *dev;
	uint32_t led;
	struct k_timer blink_timer;
	uint32_t delay_on;
	uint32_t delay_off;
};

static struct led_blink_fallback_data blink_fallback_data[CONFIG_LED_BLINK_FALLBACK_COUNT];

static void led_blink_fallback_timer_cb(struct k_timer *timer_id)
{
	struct led_blink_fallback_data *data =
		CONTAINER_OF(timer_id, struct led_blink_fallback_data, blink_timer);
	const struct led_driver_api *api = (const struct led_driver_api *)data->dev->api;

	if (k_timer_user_data_get(timer_id) == LED_ON) {
		api->off(data->dev, data->led);
		k_timer_user_data_set(&data->blink_timer, LED_OFF);
		k_timer_start(timer_id, K_MSEC(data->delay_off), K_NO_WAIT);
	} else {
		api->on(data->dev, data->led);
		k_timer_user_data_set(&data->blink_timer, LED_ON);
		k_timer_start(timer_id, K_MSEC(data->delay_on), K_NO_WAIT);
	}
}

static struct led_blink_fallback_data *led_fallback_get_data(const struct device *dev, uint32_t led)
{
	for (int i = 0; i < CONFIG_LED_BLINK_FALLBACK_COUNT; i++) {
		struct led_blink_fallback_data *data = &blink_fallback_data[i];

		if (data->dev == dev && data->led == led) {
			return data;
		}

		if (data->dev == NULL) {
			return data;
		}
	}

	return NULL;
}

int led_blink_fallback(const struct device *dev, uint32_t led, uint32_t delay_on,
		       uint32_t delay_off)
{
	struct led_blink_fallback_data *data = led_fallback_get_data(dev, led);

	if (!data) {
		return -ENODEV;
	}

	if (delay_on == 0 && delay_off == 0) {
		if (!data->dev) {
			return -ENODEV;
		}
		k_timer_stop(&data->blink_timer);
		data->dev = NULL;
		return 0;
	}

	if (data->dev) {
		data->delay_on = delay_on;
		data->delay_off = delay_off;
		return 0;
	}

	data->dev = dev;
	data->led = led;
	data->delay_on = delay_on;
	data->delay_off = delay_off;

	k_timer_init(&data->blink_timer, led_blink_fallback_timer_cb, NULL);
	k_timer_user_data_set(&data->blink_timer, LED_ON);
	k_timer_start(&data->blink_timer, K_MSEC(delay_on), K_NO_WAIT);

	return 0;
}
