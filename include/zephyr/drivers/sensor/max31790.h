/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 SILA Embedded Solutions GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of MAX31790 fan controller
 * @ingroup max31790_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX31790_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX31790_H_

/**
 * @brief Maxim MAX31790 6-Channel PWM-Output Fan RPM Controller
 * @defgroup max31790_interface MAX31790
 * @ingroup sensor_interface_ext
 *
 * The MAX31790 provides fan fault monitoring and fan speed (RPM) sensing.
 * Fan speed uses the standard @ref SENSOR_CHAN_RPM channel.
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/**
 * @brief Custom sensor channels for MAX31790.
 */
enum sensor_channel_max31790 {
	/**
	 * Fan fault status bitmask.
	 *
	 * Each bit indicates a fault for the corresponding fan (1-6).
	 * Bit 0 = fan 1 fault, bit 1 = fan 2 fault, ..., bit 5 = fan 6 fault.
	 *
	 * - @c sensor_value.val1 is the 6-bit fault mask.
	 * - @c sensor_value.val2 is unused (always 0).
	 */
	SENSOR_CHAN_MAX31790_FAN_FAULT = SENSOR_CHAN_PRIV_START,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX31790_H_ */
