/*
 * Copyright (c) 2025 Analog Devices Inc.
 * Copyright (c) 2026 Daniel Kampert
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the Analog Devices ADXL372 accelerometer.
 * @ingroup adxl372_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ADXL372_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ADXL372_H_

/**
 * @defgroup adxl372_interface ADXL372
 * @ingroup sensor_interface_ext_adi
 * @brief Analog Devices ADXL372 3-axis accelerometer
 * @{
 */

/**
 * @name FIFO mode options
 *
 * Values for the `fifo-mode` devicetree property.
 * @{
 */
#define ADXL372_FIFO_MODE_BYPASSED  0x0 /**< FIFO bypassed (disabled) */
#define ADXL372_FIFO_MODE_STREAMED  0x1 /**< Continuous stream, overwrite oldest */
#define ADXL372_FIFO_MODE_TRIGGERED 0x2 /**< Capture samples around a trigger event */
#define ADXL372_FIFO_MODE_OLD_SAVED 0x3 /**< Stop when full, keep oldest samples */
/** @} */

/**
 * @name Operating mode options
 *
 * Values for the `op-mode` devicetree property. Also settable at runtime via
 * SENSOR_ATTR_CONFIGURATION.
 * @{
 */
#define ADXL372_OP_MODE_STANDBY             0x0 /**< Lowest power, detection disabled */
#define ADXL372_OP_MODE_WAKE_UP             0x1 /**< Duty-cycled low-power detection */
#define ADXL372_OP_MODE_INSTANT_ON          0x2 /**< Always-on threshold detection */
#define ADXL372_OP_MODE_FULL_BW_MEASUREMENT 0x3 /**< Continuous measurement, highest power */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ADXL372_H_ */
