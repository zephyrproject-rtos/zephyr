/*
 * Copyright (c) 2026 Kasper Sloth
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of the generic PWM fan driver
 * @ingroup pwm_fan_interface
 *
 * This exposes an API to control the fan speed, which is specific to the
 * fan actuator and cannot be expressed within the sensor driver
 * abstraction.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_FAN_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_FAN_H_

/**
 * @defgroup pwm_fan_interface PWM fan
 * @ingroup sensor_interface_ext
 * @brief Generic PWM fan
 *
 * The generic PWM fan is driven by a PWM channel and may optionally
 * reference a tachometer sensor for RPM feedback on the standard
 * @ref SENSOR_CHAN_RPM channel.
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/**
 * @brief Custom sensor attributes for the generic PWM fan.
 */
enum sensor_attribute_fan {
	/**
	 * Fan speed setpoint expressed as a percentage.
	 *
	 * The percentage (0 to 100) is passed in the @c val1 field of the
	 * sensor_value on the @ref SENSOR_CHAN_RPM channel.
	 */
	SENSOR_ATTR_FAN_SPEED = SENSOR_ATTR_PRIV_START,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_FAN_H_ */
