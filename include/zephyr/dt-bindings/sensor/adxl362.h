/*
 * Copyright (c) 2025 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the Analog Devices ADXL362 accelerometer.
 * @ingroup adxl362_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADI_ADX362_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADI_ADX362_H_

/**
 * @defgroup adxl362_interface ADXL362
 * @ingroup sensor_interface_ext_adi
 * @brief Analog Devices ADXL362 3-axis accelerometer
 * @{
 */

/**
 * @name FIFO mode options
 *
 * Values for the `fifo-mode` devicetree property.
 * @{
 */
#define ADXL362_FIFO_MODE_DISABLED     0x0 /**< FIFO disabled */
#define ADXL362_FIFO_MODE_OLDEST_SAVED 0x1 /**< Stop when full, keep oldest samples */
#define ADXL362_FIFO_MODE_STREAM       0x2 /**< Continuous stream, overwrite oldest */
#define ADXL362_FIFO_MODE_TRIGGERED    0x3 /**< Capture samples around a trigger event */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADI_ADX362_H_ */
