/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of STC31 sensor
 * @ingroup stc31_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_STC31_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_STC31_H_

/**
 * @brief Sensirion STC31 CO<sub>2</sub> sensor
 * @defgroup stc31_interface STC31
 * @ingroup sensor_interface_ext_sensirion
 * @{
 */

#include <zephyr/drivers/sensor.h>

/**
 * @brief Custom sensor attributes for STC31
 */
enum sensor_attribute_stc31 {
	/**
	 * Relative humidity compensation input.
	 *
	 * Unit: %RH. Written to the sensor for concentration compensation.
	 * When never set after power-up/reset, the sensor assumes 0 %RH.
	 */
	SENSOR_ATTR_STC31_REL_HUMIDITY = SENSOR_ATTR_PRIV_START,
	/**
	 * External temperature compensation input.
	 *
	 * Unit: deg C. Prefer a more accurate external sensor (e.g. SHT4x).
	 * When never set, the sensor uses its internal temperature signal.
	 */
	SENSOR_ATTR_STC31_TEMPERATURE,
	/**
	 * Ambient pressure compensation input.
	 *
	 * Unit: mBar. When never set after power-up/reset, 1013 mBar is assumed.
	 */
	SENSOR_ATTR_STC31_PRESSURE,
	/**
	 * Binary gas / measurement mode argument for command 0x3615.
	 *
	 * See :c:macro:`STC31_DT_BINARY_GAS_*` in
	 * :file:`zephyr/dt-bindings/sensor/stc31.h`.
	 */
	SENSOR_ATTR_STC31_BINARY_GAS,
};

/**
 * @brief Perform a forced recalibration (FRC).
 *
 * @param dev Pointer to the sensor device
 * @param reference_ppm Reference CO2 concentration in ppm
 *        (converted internally to the sensor vol% encoding)
 *
 * @return 0 if successful, negative errno code if failure.
 */
int stc31_forced_recalibration(const struct device *dev, uint16_t reference_ppm);

/**
 * @brief Soft-reset the STC31 via I2C general call.
 *
 * Blocks for the sensor soft-reset time, then restores the last configured
 * binary gas selection.
 *
 * @param dev Pointer to the sensor device
 *
 * @return 0 if successful, negative errno code if failure.
 */
int stc31_soft_reset(const struct device *dev);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_STC31_H_ */
