/*
 * Copyright (c) 2025 Psicontrol N.V.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for HDC302X Temperature Sensors
 * @ingroup hdc302x_interface
 *
 * This exposes attributes for the HDC302X which can be used for
 * setting the Low power parameters.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_TI_HDC302X_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_TI_HDC302X_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Texas Instruments HDC302x humidity and temperature sensors
 * @defgroup hdc302x_interface HDC302X
 * @ingroup sensor_interface_ext_ti
 * @{
 */

/** @brief Custom sensor attributes for HDC302X */
enum sensor_attribute_hdc302x {
	/**
	 * Sensor low power mode, one of @ref sensor_power_mode_hdc302x.
	 *
	 * See the datasheet for more information on the different modes.
	 */
	SENSOR_ATTR_POWER_MODE = SENSOR_ATTR_PRIV_START + 1,

	/** Sensor automatic measurement mode, one of @ref sensor_measurement_interval_hdc302x. */
	SENSOR_ATTR_INTEGRATION_TIME,
	/** Sensor status register */
	SENSOR_ATTR_STATUS_REGISTER,
	/** Sensor heater level (0-14) */
	SENSOR_ATTR_HEATER_LEVEL,
};

/** @brief Power modes for the HDC302X @ref SENSOR_ATTR_POWER_MODE attribute */
enum sensor_power_mode_hdc302x {
	HDC302X_SENSOR_POWER_MODE_0, /**< Power mode 0 (lowest noise) */
	HDC302X_SENSOR_POWER_MODE_1, /**< Power mode 1 */
	HDC302X_SENSOR_POWER_MODE_2, /**< Power mode 2 */
	HDC302X_SENSOR_POWER_MODE_3, /**< Power mode 3 (lowest power) */

	HDC302X_SENSOR_POWER_MODE_MAX /**< Number of power modes */
};

/** @brief Measurement intervals for the HDC302X @ref SENSOR_ATTR_INTEGRATION_TIME attribute */
enum sensor_measurement_interval_hdc302x {
	HDC302X_SENSOR_MEAS_INTERVAL_MANUAL, /**< Manual mode */
	HDC302X_SENSOR_MEAS_INTERVAL_0_5,    /**< 1 measurement every 2 seconds */
	HDC302X_SENSOR_MEAS_INTERVAL_1,      /**< 1 measurement per second */
	HDC302X_SENSOR_MEAS_INTERVAL_2,      /**< 2 measurements per second */
	HDC302X_SENSOR_MEAS_INTERVAL_4,      /**< 4 measurements per second */
	HDC302X_SENSOR_MEAS_INTERVAL_10,     /**< 10 measurements per second */

	HDC302X_SENSOR_MEAS_INTERVAL_MAX /**< Number of measurement intervals */
};

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_TI_HDC302X_H_ */
