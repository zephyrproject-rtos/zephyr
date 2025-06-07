/*
 * Copyright (c) 2025 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/led.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

struct led_blink_fallback_data {
	const struct device *dev;
	uint32_t led;
	struct k_work_delayable work;
	uint32_t delay_on;
	uint32_t delay_off;
};

static void led_blink_fallback_off(struct k_work *work);

static K_MUTEX_DEFINE(led_blink_fallback_mutex);
static struct led_blink_fallback_data blink_fallback_data[CONFIG_LED_BLINK_FALLBACK_COUNT];

static void led_blink_fallback_on(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct led_blink_fallback_data *data =
		CONTAINER_OF(dwork, struct led_blink_fallback_data, work);

	const struct led_driver_api *api = (const struct led_driver_api *)data->dev->api;

	if (api->on) {
		api->on(data->dev, data->led);
	} else {
		api->set_brightness(data->dev, data->led, LED_BRIGTHNESS_MAX);
	}
	k_work_init_delayable(&data->work, led_blink_fallback_off);
	k_work_schedule(&data->work, K_MSEC(data->delay_on));
}

static void led_blink_fallback_off(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct led_blink_fallback_data *data =
		CONTAINER_OF(dwork, struct led_blink_fallback_data, work);

	const struct led_driver_api *api = (const struct led_driver_api *)data->dev->api;

	if (api->off) {
		api->off(data->dev, data->led);
	} else {
		api->set_brightness(data->dev, data->led, 0);
	}
	k_work_init_delayable(&data->work, led_blink_fallback_on);
	k_work_schedule(&data->work, K_MSEC(data->delay_off));
}

static struct led_blink_fallback_data *led_fallback_get_data(const struct device *dev, uint32_t led)
{
	/*
	 * Find the first unused blink_fallback_data or
	 * if the led is already bliking, return the currently used blink_fallback_data.
	 */
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
	const struct led_driver_api *api = (const struct led_driver_api *)dev->api;
	struct led_blink_fallback_data *data;
	int ret = 0;

	/* Make sure the expected API are supported before we use them */
	if (!api->set_brightness && (!api->on || !api->off)) {
		return -ENOSYS;
	}

	k_mutex_lock(&led_blink_fallback_mutex, K_FOREVER);
	data = led_fallback_get_data(dev, led);
	if (!data) {
		ret = -ENOMEM;
		goto out;
	}

	/* Stop blinking, more likely when we use led_on or led_off */
	if (delay_on == 0 && delay_off == 0) {
		struct k_work_sync sync;

		if (!data->dev) {
			ret = -ENODEV;
			goto out;
		}

		k_work_cancel_delayable_sync(&data->work, &sync);
		data->dev = NULL;
		goto out;
	}

	/* Only update the timings if the LED is already blinking */
	if (data->dev) {
		data->delay_on = delay_on;
		data->delay_off = delay_off;
		goto out;
	}

	data->dev = dev;
	data->led = led;
	data->delay_on = delay_on;
	data->delay_off = delay_off;

	k_work_init_delayable(&data->work, led_blink_fallback_on);
	k_work_schedule(&data->work, K_NO_WAIT);

out:
	k_mutex_unlock(&led_blink_fallback_mutex);
	return ret;
}
