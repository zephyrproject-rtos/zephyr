/*
 * Copyright (c) 2026 RAKwireless Technology Limited
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
	 * Temperature offset for the onboard RH/T sensor.
	 *
	 * Unit: °C. The driver converts to the device format (0.01 °C ticks).
	 */
	SENSOR_ATTR_SCD30_TEMPERATURE_OFFSET = SENSOR_ATTR_PRIV_START,
	/**
	 * Altitude compensation in metres above sea level.
	 */
	SENSOR_ATTR_SCD30_SENSOR_ALTITUDE,
	/**
	 * Ambient pressure compensation in mBar.
	 *
	 * Range: 0 (disabled) or 700–1400. Writing this restarts continuous
	 * measurement with the new pressure argument.
	 */
	SENSOR_ATTR_SCD30_AMBIENT_PRESSURE,
	/**
	 * Automatic self-calibration enable (1) or disable (0).
	 *
	 * Default after power-up is disabled.
	 */
	SENSOR_ATTR_SCD30_AUTOMATIC_CALIB_ENABLE,
	/**
	 * Continuous measurement interval in seconds.
	 *
	 * Range: 2–1800. Default: 2.
	 */
	SENSOR_ATTR_SCD30_MEASUREMENT_INTERVAL,
};

/**
 * @brief Perform a forced recalibration (FRC).
 *
 * Operate the sensor in continuous mode at a 2 s interval for at least two
 * minutes in a stable environment before calling this function.
 *
 * @param dev Pointer to the sensor device
 * @param target_concentration Reference CO2 concentration in ppm (400–2000)
 *
 * @return 0 if successful, negative errno code if failure.
 */
int scd30_forced_recalibration(const struct device *dev, uint16_t target_concentration);

/**
 * @brief Soft-reset the SCD30.
 *
 * Forces the same state as after power-up. Blocks for the sensor boot time.
 *
 * @param dev Pointer to the sensor device
 *
 * @return 0 if successful, negative errno code if failure.
 */
int scd30_soft_reset(const struct device *dev);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_SCD30_H_ */
