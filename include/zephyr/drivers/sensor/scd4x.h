/*
 * Copyright (c) 2024 Jan Fäh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_SCD4X_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_SCD4X_H_

#include <zephyr/drivers/sensor.h>

enum sensor_attribute_scd4x {
	/* Offset temperature: Toffset_actual = Tscd4x – Treference + Toffset_previous
	 * 0 - 20°C
	 */
	SENSOR_ATTR_SCD4X_TEMPERATURE_OFFSET = SENSOR_ATTR_PRIV_START,
	/* Altidude of the sensor;
	 * 0 - 3000m
	 */
	SENSOR_ATTR_SCD4X_SENSOR_ALTITUDE,
	/* Ambient pressure in hPa
	 * 700 - 1200hPa
	 */
	SENSOR_ATTR_SCD4X_AMBIENT_PRESSURE,
	/* Set the current state (enabled: 1 / disabled: 0).
	 * Default: enabled.
	 */
	SENSOR_ATTR_SCD4X_AUTOMATIC_CALIB_ENABLE,
	/* Set the initial period for automatic self calibration correction in hours. Allowed values
	 * are integer multiples of 4 hours.
	 * Default: 44
	 */
	SENSOR_ATTR_SCD4X_SELF_CALIB_INITIAL_PERIOD,
	/* Set the standard period for automatic self calibration correction in hours. Allowed
	 * values are integer multiples of 4 hours. Default: 156
	 */
	SENSOR_ATTR_SCD4X_SELF_CALIB_STANDARD_PERIOD,
};

/**
 * @brief Performs a forced recalibration.
 *
 * Operate the SCD4x in the operation mode for at least 3 minutes in an environment with a
 * homogeneous and constant CO2 concentration. Otherwise the recalibratioin will fail. The sensor
 * must be operated at the voltage desired for the application when performing the FRC sequence.
 *
 * @param dev Pointer to the sensor device
 * @param target_concentration Reference CO2 concentration.
 * @param frc_correction Previous differences from the target concentration
 *
 * @return 0 if successful, negative errno code if failure.
 */
int scd4x_forced_recalibration(const struct device *dev, uint16_t target_concentration,
			       uint16_t *frc_correction);

/**
 * @brief Performs a self test.
 *
 * The self_test command can be used as an end-of-line test to check the sensor functionality
 *
 * @param dev Pointer to the sensor device
 *
 * @return 0 if successful, negative errno code if failure.
 */
int scd4x_self_test(const struct device *dev);

/**
 * @brief Performs a self test.
 *
 * The persist_settings command can be used to save the actual configuration. This command
 * should only be sent when persistence is required and if actual changes to the configuration have
 * been made. The EEPROM is guaranteed to withstand at least 2000 write cycles
 *
 * @param dev Pointer to the sensor device
 *
 * @return 0 if successful, negative errno code if failure.
 */
int scd4x_persist_settings(const struct device *dev);

/**
 * @brief Performs a factory reset.
 *
 * The perform_factory_reset command resets all configuration settings stored in the EEPROM and
 * erases the FRC and ASC algorithm history.
 *
 * @param dev Pointer to the sensor device
 *
 * @return 0 if successful, negative errno code if failure.
 */
int scd4x_factory_reset(const struct device *dev);

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_SCD4X_H_ */
