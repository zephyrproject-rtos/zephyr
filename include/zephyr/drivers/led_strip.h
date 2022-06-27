/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for controlling linear strips of LEDs.
 *
 * This library abstracts the chipset drivers for individually
 * addressable strips of LEDs.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_LED_STRIP_H_
#define ZEPHYR_INCLUDE_DRIVERS_LED_STRIP_H_

/**
 * @brief LED Strip Interface
 * @defgroup led_strip_interface LED Strip Interface
 * @ingroup io_interfaces
 * @{
 */

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
 * @brief LED strip driver API
 *
 * This is the mandatory API any LED strip driver needs to expose.
 */
struct led_strip_driver_api {
	led_api_update_rgb update_rgb;
	led_api_update_channels update_channels;
};

/**
 * @brief Update an LED strip made of RGB pixels
 *
 * Important:
 *     This routine may overwrite @a pixels.
 *
 * This routine immediately updates the strip display according to the
 * given pixels array.
 *
 * @param dev LED strip device
 * @param pixels Array of pixel data
 * @param num_pixels Length of pixels array
 * @return 0 on success, negative on error
 * @warning May overwrite @a pixels
 */
static inline int led_strip_update_rgb(const struct device *dev,
				       struct led_rgb *pixels,
				       size_t num_pixels) {
	const struct led_strip_driver_api *api =
		(const struct led_strip_driver_api *)dev->api;

	return api->update_rgb(dev, pixels, num_pixels);
}

/**
 * @brief Update an LED strip on a per-channel basis.
 *
 * Important:
 *     This routine may overwrite @a channels.
 *
 * This routine immediately updates the strip display according to the
 * given channels array. Each channel byte corresponds to an
 * individually addressable color channel or LED. Channels
 * are updated linearly in strip order.
 *
 * @param dev LED strip device
 * @param channels Array of per-channel data
 * @param num_channels Length of channels array
 * @return 0 on success, negative on error
 * @warning May overwrite @a channels
 */
static inline int led_strip_update_channels(const struct device *dev,
					    uint8_t *channels,
					    size_t num_channels) {
	const struct led_strip_driver_api *api =
		(const struct led_strip_driver_api *)dev->api;

	return api->update_channels(dev, channels, num_channels);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif	/* ZEPHYR_INCLUDE_DRIVERS_LED_STRIP_H_ */
