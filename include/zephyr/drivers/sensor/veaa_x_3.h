/*
 * Copyright (c) 2025 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of VEAA X-3 sensor
 * @ingroup veaa_x_3_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_VEAA_X_3_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_VEAA_X_3_H_

/**
 * @brief Festo VEAA X-3 pressure regulator
 * @defgroup veaa_x_3_interface VEAA X-3
 * @ingroup sensor_interface_ext
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/**
 * @brief Custom sensor attributes for VEAA X-3
 */
enum sensor_attribute_veaa_x_3 {
	/** Set pressure setpoint value in kPa. */
	SENSOR_ATTR_VEAA_X_3_SETPOINT = SENSOR_ATTR_PRIV_START,
	/**
	 * Supported pressure range in kPa.
	 *
	 * sensor_value.val1 and sensor_value.val2 are the minimum and maximum supported pressure,
	 * respectively.
	 */
	SENSOR_ATTR_VEAA_X_3_RANGE,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_VEAA_X_3_H_ */
