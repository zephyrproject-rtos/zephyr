/*
 * Copyright (c) 2025 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for ADXL362 Devicetree constants
 * @ingroup adxl362_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADI_ADX362_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADI_ADX362_H_

/**
 * @defgroup adxl362_interface ADXL362
 * @ingroup sensor_interface_ext
 * @brief ADXL362 3-axis accelerometer
 * @{
 */

/**
 * @defgroup adxl362_fifo_mode FIFO mode options
 * @{
 */
#define ADXL362_FIFO_MODE_DISABLED        0x0 /**< FIFO disabled */
#define ADXL362_FIFO_MODE_OLDEST_SAVED    0x1 /**< Oldest saved mode */
#define ADXL362_FIFO_MODE_STREAM          0x2 /**< Stream mode */
#define ADXL362_FIFO_MODE_TRIGGERED       0x3 /**< Triggered mode */

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADI_ADX362_H_ */
