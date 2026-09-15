/*
 * Copyright (c) 2026 Dotcom IoT LLP
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public header for AS7341 spectral sensor.
 * @ingroup sensor_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_AS7341_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_AS7341_H_

#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AS7341 spectral sensor channels.
 *
 * Extends sensor_channel starting from SENSOR_CHAN_PRIV_START.
 */
enum sensor_channel_as7341 {
	/** F1 filter, 415 nm (violet) */
	SENSOR_CHAN_AS7341_415NM_F1 = SENSOR_CHAN_PRIV_START,

	/** F2 filter, 445 nm (blue) */
	SENSOR_CHAN_AS7341_445NM_F2,

	/** F3 filter, 480 nm (cyan) */
	SENSOR_CHAN_AS7341_480NM_F3,

	/** F4 filter, 515 nm (green) */
	SENSOR_CHAN_AS7341_515NM_F4,

	/** Clear channel (pass 1) */
	SENSOR_CHAN_AS7341_CLEAR_0,

	/** Near-infrared channel (pass 1) */
	SENSOR_CHAN_AS7341_NIR_0,

	/** F5 filter, 555 nm (green) */
	SENSOR_CHAN_AS7341_555NM_F5,

	/** F6 filter, 590 nm (yellow) */
	SENSOR_CHAN_AS7341_590NM_F6,

	/** F7 filter, 630 nm (red) */
	SENSOR_CHAN_AS7341_630NM_F7,

	/** F8 filter, 680 nm (red) */
	SENSOR_CHAN_AS7341_680NM_F8,

	/** Clear channel (pass 2) */
	SENSOR_CHAN_AS7341_CLEAR,

	/** Near-infrared channel (pass 2) */
	SENSOR_CHAN_AS7341_NIR,

	/** Flicker detection frequency in Hz */
	SENSOR_CHAN_AS7341_FLICKER,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_AS7341_H_ */
