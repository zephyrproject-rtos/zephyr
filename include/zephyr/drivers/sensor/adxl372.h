/*
 * Copyright (c) 2018 Analog Devices Inc.
 * Copyright (c) 2026 Daniel Kampert
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file adxl372.h
 * @brief Header file for extended sensor API of ADXL372 sensor
 *
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADXL372_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADXL372_H_

#include <zephyr/drivers/sensor.h>

/**
 * @brief ADXL372 operating modes.
 *
 * Set via SENSOR_ATTR_CONFIGURATION, passed in the sensor_value.val1 field.
 * Applies to the whole device, independent of sensor_channel.
 */
enum adxl372_op_mode {
	/** Lowest-power mode. Activity/inactivity detection is disabled. */
	ADXL372_STANDBY,
	/**
	 * Duty-cycled low-power mode: the device sleeps between samples and
	 * wakes up periodically to compare acceleration against the
	 * instant-on threshold. The wake-up rate is set via
	 * SENSOR_ATTR_SAMPLING_FREQUENCY while this mode is active.
	 */
	ADXL372_WAKE_UP,
	/** Instant-on threshold detection, without duty-cycling. */
	ADXL372_INSTANT_ON,
	/**
	 * Continuous measurement at the configured output data rate. This is
	 * the highest-power mode. The output data rate is set via
	 * SENSOR_ATTR_SAMPLING_FREQUENCY while this mode is active.
	 */
	ADXL372_FULL_BW_MEASUREMENT,
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADXL372_H_ */
