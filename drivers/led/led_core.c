/*
 * Copyright (c) 2025 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/led.h>

int z_impl_led_on(const struct device *dev, uint32_t led)
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

int z_impl_led_off(const struct device *dev, uint32_t led)
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

int z_impl_led_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	const struct led_driver_api *api = (const struct led_driver_api *)dev->api;

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
		return -ENOSYS;
	}
	return api->blink(dev, led, delay_on, delay_off);
}
