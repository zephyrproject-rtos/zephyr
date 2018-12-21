/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <led.h>

int _impl_led_blink(struct device *dev, u32_t led,
		    u32_t delay_on, u32_t delay_off)
{
	const struct led_driver_api *api = dev->driver_api;

	return api->blink(dev, led, delay_on, delay_off);
}

int _impl_led_set_brightness(struct device *dev, u32_t led,
			     u8_t value)
{
	const struct led_driver_api *api = dev->driver_api;

	return api->set_brightness(dev, led, value);
}

int _impl_led_on(struct device *dev, u32_t led)
{
	const struct led_driver_api *api = dev->driver_api;

	return api->on(dev, led);
}

int _impl_led_off(struct device *dev, u32_t led)
{
	const struct led_driver_api *api = dev->driver_api;

	return api->off(dev, led);
}
