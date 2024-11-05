/*
 * Copyright (c) 2024, MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for Phosense XBR818 10 GHz Radar
 *
 * This exposes 4 additional attributes used to configure the IC
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_XBR818_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_XBR818_H_

#ifdef __cplusplus
extern "C" {
#endif

enum sensor_attribute_xbr818 {
	/*!
	 * Time of received activity before output is triggered, in seconds
	 */
	SENSOR_ATTR_XBR818_DELAY_TIME = SENSOR_ATTR_PRIV_START,
	/*!
	 * How long output stays triggered after no more activity is detected, in seconds
	 */
	SENSOR_ATTR_XBR818_LOCK_TIME,
	/*!
	 * Noise floor Threshold for Radar, 16 first LSBs of the integer part.
	 */
	SENSOR_ATTR_XBR818_NOISE_FLOOR,
	/*!
	 * RF Power for Radar, 0 to 7, LSB of the integer part.
	 */
	SENSOR_ATTR_XBR818_RF_POWER
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_XBR818_H_ */
