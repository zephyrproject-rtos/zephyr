/*
 * Copyright (c) 2025 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of ExplorIR-M sensor
 * @ingroup explorir_m_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_EXPLORIR_M_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_EXPLORIR_M_H_

/**
 * @defgroup explorir_m_interface ExplorIR-M
 * @ingroup sensor_interface_ext
 * @brief Gas Sensing Solutions ExplorIR-M CO<sub>2</sub> sensor
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/**
 * Custom sensor attributes for ExplorIR-M CO2 sensor
 */
enum sensor_attribute_explorir_m {
	/**
	 * Sensor's integrated low-pass filter.
	 *
	 * Allowed values: 0-65535 (default: 16)
	 */
	SENSOR_ATTR_EXPLORIR_M_FILTER = SENSOR_ATTR_PRIV_START,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_EXPLORIR_M_H_ */
