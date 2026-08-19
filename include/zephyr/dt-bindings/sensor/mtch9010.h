/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the Microchip MTCH9010 sensor.
 * @ingroup mtch9010_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_MTCH9010_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_MTCH9010_H_

/**
 * @defgroup mtch9010_interface MTCH9010
 * @ingroup sensor_interface_ext_microchip
 * @brief Microchip MTCH9010 capacitive/conductive liquid-level sensor
 * @{
 */

/**
 * @name Operating mode
 *
 * Values for the `operating-mode` devicetree property.
 * @{
 */
#define MTCH9010_CAPACITIVE 0x0 /**< Capacitive sensing */
#define MTCH9010_CONDUCTIVE 0x1 /**< Conductive sensing */
/** @} */

/**
 * @name UART output data format
 *
 * Values for the `extended-output-format` devicetree property.
 * @{
 */
#define MTCH9010_OUTPUT_FORMAT_DELTA 0x0                 /**< Delta from reference */
#define MTCH9010_OUTPUT_FORMAT_CURRENT 0x1               /**< Current measurement */
#define MTCH9010_OUTPUT_FORMAT_BOTH 0x2                  /**< Delta and current */
#define MTCH9010_OUTPUT_FORMAT_MPLAB_DATA_VISUALIZER 0x3 /**< MPLAB Data Visualizer stream */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_MTCH9010_H_ */
