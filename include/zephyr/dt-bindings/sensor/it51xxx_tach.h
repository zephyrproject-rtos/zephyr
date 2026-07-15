/*
 * Copyright (c) 2025 ITE Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the ITE IT51xxx tachometer.
 * @ingroup it51xxx_tach_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_IT51XXX_TACH_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_IT51XXX_TACH_H_

/**
 * @defgroup it51xxx_tach_interface IT51XXX tachometer
 * @ingroup sensor_interface_ext_ite
 * @brief ITE IT51xxx tachometer
 * @{
 */

/**
 * @name Tachometer input pin
 *
 * Values for the `input-pin` devicetree property.
 * @{
 */
#define IT51XXX_TACH_INPUT_PIN_A	0 /**< Tachometer input from pin A */
#define IT51XXX_TACH_INPUT_PIN_B	1 /**< Tachometer input from pin B */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_IT51XXX_TACH_H_ */
