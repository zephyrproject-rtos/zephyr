/*
 * Copyright (c) 2026 Analog Devices Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file adxl355.h
 * @brief Header file for extended sensor API of ADXL355 sensor
 *
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADXL355_ADXL355_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADXL355_ADXL355_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/dt-bindings/sensor/adxl355.h>

/**
 * @brief ADXL355-specific sensor attributes
 *
 * These attributes extend the standard sensor_attribute enum
 * to support ADXL355-specific configuration options.
 */
enum sensor_attribute_adxl355 {
	/** FIFO watermark level */
	SENSOR_ATTR_ADXL355_FIFO_WATERMARK = SENSOR_ATTR_PRIV_START,
	/** Highpass filter corner */
	SENSOR_ATTR_ADXL355_HPF_CORNER,
	/** External clock setting */
	SENSOR_ATTR_ADXL355_EXT_CLK,
	/** External sync setting */
	SENSOR_ATTR_ADXL355_EXT_SYNC,
	/** I2C high speed mode */
	SENSOR_ATTR_ADXL355_I2C_HS,
	/** Interrupt polarity */
	SENSOR_ATTR_ADXL355_INT_POL,
	/** Activity threshold */
	SENSOR_ATTR_ADXL355_ACTIVITY_THRESHOLD,
	/** Activity count */
	SENSOR_ATTR_ADXL355_ACTIVITY_COUNT,
	/** Activity enable for X axis */
	SENSOR_ATTR_ADXL355_ACTIVITY_ENABLE_X,
	/** Activity enable for Y axis */
	SENSOR_ATTR_ADXL355_ACTIVITY_ENABLE_Y,
	/** Activity enable for Z axis */
	SENSOR_ATTR_ADXL355_ACTIVITY_ENABLE_Z,
};

/**
 * @brief ADXL355 High Pass Filter corner frequency options
 *
 * These options correspond to the DT binding values for HPF corner settings.
 */
enum adxl355_hpf_corner {
	/** High-pass filter disabled */
	ADXL355_HPF_OFF = ADXL355_DT_HPF_OFF,
	/** High-pass filter corner at 24.7e-4 times ODR */
	ADXL355_HPF_24_7e_4,
	/** High-pass filter corner at 6.208e-4 times ODR */
	ADXL355_HPF_6_208e_4,
	/** High-pass filter corner at 1.5545e-4 times ODR */
	ADXL355_HPF_1_5545e_4,
	/** High-pass filter corner at 0.3862e-4 times ODR */
	ADXL355_HPF_0_3862e_4,
	/** High-pass filter corner at 0.0954e-4 times ODR */
	ADXL355_HPF_0_0954e_4,
	/** High-pass filter corner at 0.0238e-4 times ODR */
	ADXL355_HPF_0_0238e_4,
};
#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADXL355_ADXL355_H_ */
