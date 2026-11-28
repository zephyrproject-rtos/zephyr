/*
 * Copyright (c) 2024 Jan Fäh
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of SCD30 sensor
 * @ingroup scd30_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_SCD30_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_SCD30_H_

/**
 * @brief Sensirion SCD30 CO<sub>2</sub> sensor
 * @defgroup scd30_interface SCD30
 * @ingroup sensor_interface_ext_sensirion
 * @{
 */

#include <zephyr/drivers/sensor.h>

/**
 * @brief Custom sensor attributes for SCD30
 */
enum sensor_attribute_scd30 {
	/**
	 * Temperature offset
	 *
	 * 0 - 654.99°C
	 */
	SENSOR_ATTR_SCD30_TEMPERATURE_OFFSET = SENSOR_ATTR_PRIV_START,
	/**
	 * Altitude of the sensor
	 *
	 * 0 - 65535m
	 */
	SENSOR_ATTR_SCD30_SENSOR_ALTITUDE,
	/**
	 * Ambient pressure in hPa/mBar
	 *
	 * 700 - 1400hPa/mBar
	 * 0 disables compensation.
	 */
	SENSOR_ATTR_SCD30_AMBIENT_PRESSURE,
	/**
	 * Automatic calibration enable (enabled: 1 / disabled: 0).
	 *
	 * A minimum period of 7 days is needed with exposure to fresh air at least 1 hour a day.
	 *
	 * Default: disabled.
	 */
	SENSOR_ATTR_SCD30_AUTOMATIC_CALIB_ENABLE,
	/**
	 * Forced recalibration value
	 *
	 * Operate the SCD30 with a measurement period 2s for at least 2 minutes in an environment
	 * with a homogeneous and constant CO2 concentration before setting this value.
	 * Applying FRC permanently updates the CO2 calibration curve.
	 * Reading the attribute returns the last reference value used,
	 * which resets to 400ppm after power loss. FRC and ASC override each other's corrections.
	 *
	 * 400 - 2000ppm
	 */
	SENSOR_ATTR_SCD30_FORCED_RECALIBRATION_VALUE,
};

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_SCD30_H_ */
