/*
 * Copyright (c) 2021 G-Technologies Sdn. Bhd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of MH-Z19B sensor
 * @ingroup mhz19b_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MHZ19B_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MHZ19B_H_

/**
 * @defgroup mhz19b_interface MH-Z19B
 * @ingroup sensor_interface_ext
 * @brief Winsen MH-Z19B CO<sub>2</sub> sensor
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/**
 * @brief Custom sensor attributes for MH-Z19B
 */
enum sensor_attribute_mhz19b {
	/**
	 * Automatic Baseline Correction Self Calibration Function.
	 *
	 * - sensor_value.val1 == 0: Disabled
	 * - sensor_value.val1 == 1: Enabled
	 */
	SENSOR_ATTR_MHZ19B_ABC = SENSOR_ATTR_PRIV_START,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_MHZ19B_H_ */
