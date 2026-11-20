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
	 * Unit: deg C. The interface manual programs a uint16 in ticks of
	 * 0.01 deg C, so the accepted range is 0 to 655.35 deg C.
	 */
	SENSOR_ATTR_SCD30_TEMPERATURE_OFFSET = SENSOR_ATTR_PRIV_START,
	/**
	 * Altitude compensation in metres above sea level.
	 *
	 * Range: 0 to 65535. The interface manual stores this as a uint16.
	 * It is disregarded while ambient pressure compensation is active.
	 */
	SENSOR_ATTR_SCD30_SENSOR_ALTITUDE,
	/**
	 * Ambient pressure compensation in mBar.
	 *
	 * Range: 0 (disabled) or 700-1400. Writing this restarts continuous
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
	 * Range: 2-1800. Default: 2.
	 */
	SENSOR_ATTR_SCD30_MEASUREMENT_INTERVAL,
};

/**
 * @brief Perform a forced recalibration (FRC).
 *
 * Interface manual: run continuous measurement at a 2 s interval for at least
 * two minutes in a stable environment before calling this function. The
 * command is sent while measurement is still running.
 *
 * @param dev Pointer to the sensor device
 * @param target_concentration Reference CO2 concentration in ppm (400-2000)
 *
 * @return 0 on success, -EINVAL if the concentration is outside 400..2000 ppm,
 *         -EAGAIN if continuous measurement is not running, or a negative
 *         errno from the bus.
 */
int scd30_forced_recalibration(const struct device *dev, uint16_t target_concentration);

/**
 * @brief Soft-reset the SCD30.
 *
 * Forces the same state as after power-up and blocks for the sensor boot
 * time. The measurement interval and continuous-measurement state are
 * reloaded from the sensor. Ambient pressure compensation is disabled.
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
