/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for LED driver APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_LED_H_
#error "Should only be included by zephyr/drivers/led.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_LED_INTERNAL_LED_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_LED_INTERNAL_LED_IMPL_H_

#include <errno.h>

#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_led_blink(const struct device *dev, uint32_t led,
				   uint32_t delay_on, uint32_t delay_off)
{
	const struct led_driver_api *api =
		(const struct led_driver_api *)dev->api;

	if (api->blink == NULL) {
		return -ENOSYS;
	}
	return api->blink(dev, led, delay_on, delay_off);
}

static inline int z_impl_led_get_info(const struct device *dev, uint32_t led,
				      const struct led_info **info)
{
	const struct led_driver_api *api =
		(const struct led_driver_api *)dev->api;

	if (api->get_info == NULL) {
		*info = NULL;
		return -ENOSYS;
	}
	return api->get_info(dev, led, info);
}

static inline int z_impl_led_set_brightness(const struct device *dev,
					    uint32_t led,
					    uint8_t value)
{
	const struct led_driver_api *api =
		(const struct led_driver_api *)dev->api;

	if (api->set_brightness == NULL) {
		return -ENOSYS;
	}
	return api->set_brightness(dev, led, value);
}

static inline int
z_impl_led_write_channels(const struct device *dev, uint32_t start_channel,
			  uint32_t num_channels, const uint8_t *buf)
{
	const struct led_driver_api *api =
		(const struct led_driver_api *)dev->api;

	if (api->write_channels == NULL) {
		return -ENOSYS;
	}
	return api->write_channels(dev, start_channel, num_channels, buf);
}

static inline int z_impl_led_set_channel(const struct device *dev,
					 uint32_t channel, uint8_t value)
{
	return z_impl_led_write_channels(dev, channel, 1, &value);
}

static inline int z_impl_led_set_color(const struct device *dev, uint32_t led,
				       uint8_t num_colors, const uint8_t *color)
{
	const struct led_driver_api *api =
		(const struct led_driver_api *)dev->api;

	if (api->set_color == NULL) {
		return -ENOSYS;
	}
	return api->set_color(dev, led, num_colors, color);
}

static inline int z_impl_led_on(const struct device *dev, uint32_t led)
{
	const struct led_driver_api *api =
		(const struct led_driver_api *)dev->api;

	return api->on(dev, led);
}

static inline int z_impl_led_off(const struct device *dev, uint32_t led)
{
	const struct led_driver_api *api =
		(const struct led_driver_api *)dev->api;

	return api->off(dev, led);
}

#ifdef __cplusplus
}
#endif

#endif	/* ZEPHYR_INCLUDE_DRIVERS_LED_INTERNAL_LED_IMPL_H_ */
