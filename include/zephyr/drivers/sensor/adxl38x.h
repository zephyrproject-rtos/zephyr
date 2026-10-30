/*
 * Copyright (c) 2026 Analog Devices Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file adxl38x.h
 * @brief Header file for the extended sensor API of the ADXL380/ADXL382 sensor
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADXL38X_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADXL38X_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/dt-bindings/sensor/adxl38x.h>

/**
 * @brief ADXL380/ADXL382-specific sensor attributes
 *
 * These attributes extend the standard sensor_attribute enum with
 * ADXL38X-specific configuration options.
 */
enum sensor_attribute_adxl38x {
	/** Operating mode (see @ref adxl38x_op_mode) */
	SENSOR_ATTR_ADXL38X_OP_MODE = SENSOR_ATTR_PRIV_START,
	/** FIFO watermark level, in sample sets */
	SENSOR_ATTR_ADXL38X_FIFO_WATERMARK,
	/** High-pass filter corner (see @ref adxl38x_hpf_corner) */
	SENSOR_ATTR_ADXL38X_HPF_CORNER,
	/** Interrupt polarity (see @ref adxl38x_int_pol) */
	SENSOR_ATTR_ADXL38X_INT_POL,
	/** Activity threshold, in m/s^2 */
	SENSOR_ATTR_ADXL38X_ACTIVITY_THRESHOLD,
	/** Activity time, in microseconds */
	SENSOR_ATTR_ADXL38X_ACTIVITY_TIME,
	/** Inactivity threshold, in m/s^2 */
	SENSOR_ATTR_ADXL38X_INACTIVITY_THRESHOLD,
	/** Inactivity time, in microseconds */
	SENSOR_ATTR_ADXL38X_INACTIVITY_TIME,
	/** Activity enable for X axis */
	SENSOR_ATTR_ADXL38X_ACTIVITY_ENABLE_X,
	/** Activity enable for Y axis */
	SENSOR_ATTR_ADXL38X_ACTIVITY_ENABLE_Y,
	/** Activity enable for Z axis */
	SENSOR_ATTR_ADXL38X_ACTIVITY_ENABLE_Z,
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADXL38X_H_ */
