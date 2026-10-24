/*
 * Copyright (c) 2023 SILA Embedded Solutions
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of MAX31865 sensor
 * @ingroup max31865_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX31865_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX31865_H_

/**
 * @brief Analog Devices MAX31865 RTD-to-digital converter
 * @defgroup max31865_interface MAX31865
 * @ingroup sensor_interface_ext_adi
 * @{
 */

/**
 * Select the RTD wiring.
 *
 * Set @c sensor_value.val1 to 1 for a 3-wire RTD, or to 0 for a 2-wire or 4-wire RTD.
 */
#define SENSOR_ATTR_MAX31865_THREE_WIRE SENSOR_ATTR_PRIV_START

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX31865_H_ */
