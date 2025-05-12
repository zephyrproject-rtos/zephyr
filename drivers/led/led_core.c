/*
 * Copyright (c) 2025 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>

#include "led_blink.h"

static int led_core_on(const struct device *dev, uint32_t led)
{
	const struct led_driver_api *api = (const struct led_driver_api *)dev->api;

	if (api->set_brightness == NULL && api->on == NULL) {
		return -ENOSYS;
	}

	if (api->on == NULL) {
		return api->set_brightness(dev, led, LED_BRIGHTNESS_MAX);
	}

	return api->on(dev, led);
}

static int led_core_off(const struct device *dev, uint32_t led)
{
	const struct led_driver_api *api = (const struct led_driver_api *)dev->api;

	if (api->set_brightness == NULL && api->off == NULL) {
		return -ENOSYS;
	}

	if (api->off == NULL) {
		return api->set_brightness(dev, led, 0);
	}

	return api->off(dev, led);
}

#ifdef CONFIG_LED_BLINK_SOFTWARE
static void led_blink_software_off(struct k_work *work);
static struct led_blink_software_data *led_blink_software_get_data(const struct device *dev,
								   uint32_t led)
{
	const struct led_driver_api *api = (const struct led_driver_api *)dev->api;

	if (!api->get_blink_data) {
		return NULL;
	}

	return api->get_blink_data(dev, led);
}

static void led_blink_software_on(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct led_blink_software_data *data =
		CONTAINER_OF(dwork, struct led_blink_software_data, work);

	led_core_on(data->dev, data->led);
	k_work_init_delayable(&data->work, led_blink_software_off);
	k_work_schedule(&data->work, K_MSEC(data->delay_on));
}

static void led_blink_software_off(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct led_blink_software_data *data =
		CONTAINER_OF(dwork, struct led_blink_software_data, work);

	led_core_off(data->dev, data->led);
	k_work_init_delayable(&data->work, led_blink_software_on);
	k_work_schedule(&data->work, K_MSEC(data->delay_off));
}

int led_blink_software_start(const struct device *dev, uint32_t led, uint32_t delay_on,
			     uint32_t delay_off)
{
	struct led_blink_software_data *data;

	data = led_blink_software_get_data(dev, led);
	if (!data) {
		return -EINVAL;
	}

	if (!delay_on && !delay_off) {
		/* Default 1Hz blinking when delay_on and delay_off are 0 */
		delay_on = 500;
		delay_off = 500;
	} else if (!delay_on) {
		/* Always off */
		led_core_off(dev, led);
	} else if (!delay_off) {
		/* Always on */
		led_core_on(dev, led);
	}

	data->dev = dev;
	data->led = led;
	data->delay_on = delay_on;
	data->delay_off = delay_off;

	k_work_init_delayable(&data->work, led_blink_software_on);
	return k_work_schedule(&data->work, K_NO_WAIT);
}

static void led_blink_software_stop(const struct device *dev, uint32_t led)
{
	struct led_blink_software_data *data;

	data = led_blink_software_get_data(dev, led);
	if (data) {
		struct k_work_sync sync;

		k_work_cancel_delayable_sync(&data->work, &sync);
	}
}
#else
static inline void led_blink_software_stop(const struct device *dev, uint32_t led)
{
}
#endif /* CONFIG_LED_BLINK_SOFTWARE */

int z_impl_led_on(const struct device *dev, uint32_t led)
{
	led_blink_software_stop(dev, led);
	return led_core_on(dev, led);
}

int z_impl_led_off(const struct device *dev, uint32_t led)
{
	led_blink_software_stop(dev, led);
	return led_core_off(dev, led);
}

int z_impl_led_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	const struct led_driver_api *api = (const struct led_driver_api *)dev->api;

	if (!value) {
		led_blink_software_stop(dev, led);
	}

	if (api->set_brightness == NULL) {
		if (api->on == NULL || api->off == NULL) {
			return -ENOSYS;
		}
	}

	if (value > LED_BRIGHTNESS_MAX) {
		return -EINVAL;
	}

	if (api->set_brightness == NULL) {
		if (value) {
			return api->on(dev, led);
		} else {
			return api->off(dev, led);
		}
	}

	return api->set_brightness(dev, led, value);
}

int z_impl_led_blink(const struct device *dev, uint32_t led, uint32_t delay_on, uint32_t delay_off)
{
	const struct led_driver_api *api = (const struct led_driver_api *)dev->api;

	if (api->blink == NULL) {
		return led_blink_software_start(dev, led, delay_on, delay_off);
	}
	return api->blink(dev, led, delay_on, delay_off);
}
