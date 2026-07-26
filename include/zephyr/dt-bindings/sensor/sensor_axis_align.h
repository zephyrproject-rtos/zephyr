/*
 * Copyright 2025 CogniPilot Foundation
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for sensor axis alignment.
 * @ingroup sensor_axis_align_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_SENSOR_AXIS_ALIGN_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_SENSOR_AXIS_ALIGN_H_

/**
 * @defgroup sensor_axis_align_interface Sensor axis alignment
 * @ingroup sensor_interface_ext
 * @brief Shared constants for remapping sensor axes in the devicetree
 * @{
 */

/**
 * @name Axis index
 *
 * Values for the `axis-align-x`, `axis-align-y`, and `axis-align-z` devicetree properties.
 * @{
 */
#define SENSOR_AXIS_ALIGN_DT_X		0 /**< X axis */
#define SENSOR_AXIS_ALIGN_DT_Y		1 /**< Y axis */
#define SENSOR_AXIS_ALIGN_DT_Z		2 /**< Z axis */
/** @} */

/**
 * @name Axis direction
 *
 * Values for the `axis-align-x-sign`, `axis-align-y-sign`, and
 * `axis-align-z-sign` devicetree properties.
 * @{
 */
#define SENSOR_AXIS_ALIGN_DT_NEG		0 /**< Negative direction */
#define SENSOR_AXIS_ALIGN_DT_POS		2 /**< Positive direction */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_SENSOR_AXIS_ALIGN_H_ */
