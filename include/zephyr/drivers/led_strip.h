/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2024 Jamie McCrae
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup led_strip_interface
 * @brief Main header file for LED strip driver API.
 *
 * This library abstracts the chipset drivers for individually
 * addressable strips of LEDs.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_LED_STRIP_H_
#define ZEPHYR_INCLUDE_DRIVERS_LED_STRIP_H_

/**
 * @brief Interfaces for LED strips.
 * @defgroup led_strip_interface LED Strip
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
 * @brief Color value for a single RGB LED.
 *
 * Individual strip drivers may ignore lower-order bits if their
 * resolution in any channel is less than a full byte.
 */
struct led_rgb {
#ifdef CONFIG_LED_STRIP_RGB_SCRATCH
	/*
	 * Pad/scratch space needed by some drivers. Users should
	 * ignore.
	 */
	uint8_t scratch;
#endif
	/** Red channel */
	uint8_t r;
	/** Green channel */
	uint8_t g;
	/** Blue channel */
	uint8_t b;
};

/**
 * @typedef led_api_update_rgb
 * @brief Callback API for updating an RGB LED strip
 *
 * @see led_strip_update_rgb() for argument descriptions.
 */
typedef int (*led_api_update_rgb)(const struct device *dev,
				  struct led_rgb *pixels,
				  size_t num_pixels);

/**
 * @typedef led_api_update_channels
 * @brief Callback API for updating channels without an RGB interpretation.
 *
 * @see led_strip_update_channels() for argument descriptions.
 */
typedef int (*led_api_update_channels)(const struct device *dev,
				       uint8_t *channels,
				       size_t num_channels);

/**
 * @typedef led_api_length
 * @brief Callback API for getting length of an LED strip.
 *
 * @see led_strip_length() for argument descriptions.
 */
typedef size_t (*led_api_length)(const struct device *dev);

/**
 * @brief LED strip driver API
 *
 * This is the mandatory API any LED strip driver needs to expose.
 */
__subsystem struct led_strip_driver_api {
	led_api_update_rgb update_rgb;
	led_api_update_channels update_channels;
	led_api_length length;
};

/**
 * @brief		Mandatory function to update an LED strip with the given RGB array.
 *
 * @param dev		LED strip device.
 * @param pixels	Array of pixel data.
 * @param num_pixels	Length of pixels array.
 *
 * @retval		0 on success.
 * @retval		-errno negative errno code on failure.
 *
 * @warning		This routine may overwrite @a pixels.
 */
static inline int led_strip_update_rgb(const struct device *dev,
				       struct led_rgb *pixels,
				       size_t num_pixels)
{
	const struct led_strip_driver_api *api =
		(const struct led_strip_driver_api *)dev->api;

	/* Allow for out-of-tree drivers that do not have this function for 2 Zephyr releases
	 * until making it mandatory, function added after Zephyr 3.6
	 */
	if (api->length != NULL) {
		/* Ensure supplied pixel size is valid for this device */
		if (api->length(dev) < num_pixels) {
			return -ERANGE;
		}
	}

	return api->update_rgb(dev, pixels, num_pixels);
}

/**
 * @brief		Optional function to update an LED strip with the given channel array
 *			(each channel byte corresponding to an individually addressable color
 *			channel or LED. Channels are updated linearly in strip order.
 *
 * @param dev		LED strip device.
 * @param channels	Array of per-channel data.
 * @param num_channels	Length of channels array.
 *
 * @retval		0 on success.
 * @retval		-ENOSYS if not implemented.
 * @retval		-errno negative errno code on other failure.
 *
 * @warning		This routine may overwrite @a channels.
 */
static inline int led_strip_update_channels(const struct device *dev,
					    uint8_t *channels,
					    size_t num_channels)
{
	const struct led_strip_driver_api *api =
		(const struct led_strip_driver_api *)dev->api;

	if (api->update_channels == NULL) {
		return -ENOSYS;
	}

	return api->update_channels(dev, channels, num_channels);
}

/**
 * @brief	Mandatory function to get chain length (in pixels) of an LED strip device.
 *
 * @param dev	LED strip device.
 *
 * @retval	Length of LED strip device.
 */
static inline size_t led_strip_length(const struct device *dev)
{
	const struct led_strip_driver_api *api =
		(const struct led_strip_driver_api *)dev->api;

	return api->length(dev);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif	/* ZEPHYR_INCLUDE_DRIVERS_LED_STRIP_H_ */
