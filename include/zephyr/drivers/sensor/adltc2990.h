/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of ADLTC2990 sensor
 * @ingroup adltc2990_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADLTC2990_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADLTC2990_H_

/**
 * @brief Analog Devices ADLTC2990 Quad I2C Voltage, Current and Temperature Monitor
 * @defgroup adltc2990_interface ADLTC2990
 * @ingroup sensor_interface_ext
 * @{
 */

#include <stdbool.h>

#include <zephyr/device.h>

/**
 * @brief Acquisition format
 */
enum adltc2990_acquisition_format {
	/** Repeated acquisition */
	ADLTC2990_REPEATED_ACQUISITION,
	/** Single shot acquisition */
	ADLTC2990_SINGLE_SHOT_ACQUISITION,
};

/**
 * @brief check if adtlc2990 is busy doing conversion
 *
 * @param dev Pointer to the adltc2990 device
 * @param is_busy Pointer to the variable to store the busy status
 *
 * @retval 0 if successful
 * @retval -EIO General input / output error.
 */
int adltc2990_is_busy(const struct device *dev, bool *is_busy);

/**
 * @brief Trigger the adltc2990 to start a measurement
 *
 * @param dev Pointer to the adltc2990 device
 * @param format The acquisition format to be used
 *
 * @retval 0 if successful
 * @retval -EIO General input / output error.
 */
int adltc2990_trigger_measurement(const struct device *dev,
				  enum adltc2990_acquisition_format format);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADLTC2990_H_ */
