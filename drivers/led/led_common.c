/*
 * SPDX-FileCopyrightText: Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <zephyr/types.h>

inline int z_impl_led_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	const struct led_driver_api *api = DEVICE_API_GET(led, dev);

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

inline int z_impl_led_on(const struct device *dev, uint32_t led)
{
	const struct led_driver_api *api = DEVICE_API_GET(led, dev);

	if (api->set_brightness == NULL && api->on == NULL) {
		return -ENOSYS;
	}

	if (api->on == NULL) {
		return api->set_brightness(dev, led, LED_BRIGHTNESS_MAX);
	}

	return api->on(dev, led);
}

inline int z_impl_led_off(const struct device *dev, uint32_t led)
{
	const struct led_driver_api *api = DEVICE_API_GET(led, dev);

	if (api->set_brightness == NULL && api->off == NULL) {
		return -ENOSYS;
	}

	if (api->off == NULL) {
		return api->set_brightness(dev, led, 0);
	}

	return api->off(dev, led);
}
