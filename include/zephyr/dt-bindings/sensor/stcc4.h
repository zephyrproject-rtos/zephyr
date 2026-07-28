/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the Sensirion STCC4 CO2 sensor.
 * @ingroup stcc4_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_STCC4_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_STCC4_H_

/**
 * @defgroup stcc4_interface STCC4
 * @ingroup sensor_interface_ext_sensirion
 * @brief Sensirion STCC4 CO2 sensor
 * @{
 */

/**
 * @name Measurement mode
 *
 * Values for the `measurement-mode` devicetree property.
 * @{
 */
#define STCC4_DT_MODE_CONTINUOUS  0 /**< Continuous measurement (~1 Hz) */
#define STCC4_DT_MODE_SINGLE_SHOT 1 /**< Single-shot measurement (500 ms each) */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_STCC4_H_ */
