/*
 * Copyright (c) 2024 Martin Kiepfer <mrmarteng@teleschirm.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public Backlight driver APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_BACKLIGHT_H_
#define ZEPHYR_INCLUDE_DRIVERS_BACKLIGHT_H_

/**
 * @brief Backlight Driver Interface
 * @defgroup backlight_interface Interface
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>

#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Backlight information structure
 *
 * This structure gathers useful information about Backlight controller.
 */
struct backlight_info {
	/** Backlight label */
	const char *label;
};

/**
 * @typedef backlight_api_set_brightness()
 * @brief Callback API for setting brightness of an Backlight
 *
 * @see backlight_set_brightness() for argument descriptions.
 */
typedef int (*backlight_api_set_brightness)(const struct device *dev, uint8_t value);

/**
 * @typedef backlight_api_get_brightness()
 * @brief Callback API for getting brightness of an Backlight
 *
 * @see backlight_set_brightness() for argument descriptions.
 */
typedef int (*backlight_api_get_brightness)(const struct device *dev, uint8_t *value);

/**
 * @typedef backlight_api_on()
 * @brief Callback API for turning on the Backlight

 * @see backlight_on() for argument descriptions.
 */
typedef int (*backlight_api_on)(const struct device *dev);

/**
 * @typedef backlight_api_off()
 * @brief Callback API for turning off Backlight
 *
 * @see backlight_off() for argument descriptions.
 */
typedef int (*backlight_api_off)(const struct device *dev);

/**
 * @brief Backlight driver API
 */
__subsystem struct backlight_driver_api {
	/* Mandatory callbacks. */
	backlight_api_on set_on;
	backlight_api_off set_off;
	/* Optional callbacks. */
	backlight_api_set_brightness set_brightness;
	backlight_api_get_brightness get_brightness;
};

/**
 * @brief Set Backlight brightness
 *
 * This optional routine sets the brightness of a Backlight to the given value.
 *
 * These should simply turn the Backlight on if @p value is nonzero, and off
 * if @p value is zero.
 *
 * @param dev Backlight device
 * @param value Brightness value to set in percent (0..100)
 * @return 0 on success, negative on error
 */
__syscall int backlight_set_brightness(const struct device *dev, uint8_t value);

static inline int z_impl_backlight_set_brightness(const struct device *dev, uint8_t value)
{
	const struct backlight_driver_api *api = (const struct backlight_driver_api *)dev->api;

	if (api->set_brightness == NULL) {
		return -ENOSYS;
	}
	return api->set_brightness(dev, value);
}

/**
 * @brief Turn on an Backlight
 *
 * This routine turns on Backlight. It will restore the previous set value
 * or the initial value automatically.
 *
 * @param dev Backlight device
 * @return 0 on success, negative on error
 */
__syscall int backlight_on(const struct device *dev);

static inline int z_impl_backlight_on(const struct device *dev)
{
	const struct backlight_driver_api *api = (const struct backlight_driver_api *)dev->api;

	return api->set_on(dev);
}

/**
 * @brief Turn off an Backlight
 *
 * This routine turns off an Backlight
 *
 * @param dev Backlight device
 * @return 0 on success, negative on error
 */
__syscall int backlight_off(const struct device *dev);

static inline int z_impl_backlight_off(const struct device *dev)
{
	const struct backlight_driver_api *api = (const struct backlight_driver_api *)dev->api;

	return api->set_off(dev);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/backlight.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_BACKLIGHT_H_ */
