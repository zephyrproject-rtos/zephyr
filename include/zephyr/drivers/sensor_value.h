/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_VALUE_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_VALUE_H_

/**
 * @file
 * @ingroup sensor_interface
 * @brief Main header file for sensor driver API.
 */

/**
 * @brief Interfaces for sensors.
 * @defgroup sensor_interface Sensor
 * @since 1.2
 * @version 1.0.0
 * @ingroup io_interfaces
 * @{
 *
 * @defgroup sensor_interface_ext Device-specific Sensor API extensions
 * @{
 * @}
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Representation of a sensor readout value.
 *
 * The value is represented as having an integer and a fractional part,
 * and can be obtained using the formula val1 + val2 * 10^(-6). Negative
 * values also adhere to the above formula, but may need special attention.
 * Here are some examples of the value representation:
 *
 *      0.5: val1 =  0, val2 =  500000
 *     -0.5: val1 =  0, val2 = -500000
 *     -1.0: val1 = -1, val2 =  0
 *     -1.5: val1 = -1, val2 = -500000
 */
struct sensor_value {
	/** Integer part of the value. */
	int32_t val1;
	/** Fractional part of the value (in one-millionth parts). */
	int32_t val2;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_VALUE_H_ */
