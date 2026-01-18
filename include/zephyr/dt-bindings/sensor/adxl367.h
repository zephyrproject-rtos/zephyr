/*
 * Copyright (c) 2025 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for ADXL367 Devicetree constants
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADI_ADX367_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADI_ADX367_H_

/**
 * @defgroup adxl367_interface ADXL367
 * @ingroup sensor_interface_ext
 * @brief ADXL367 3-axis accelerometer
 * @{
 */

/**
 * @defgroup adxl367_fifo_mode FIFO mode options
 * @{
 */
#define ADXL367_FIFO_MODE_DISABLED        0x0 /**< FIFO disabled */
#define ADXL367_FIFO_MODE_OLDEST_SAVED    0x1 /**< Oldest saved mode */
#define ADXL367_FIFO_MODE_STREAM          0x2 /**< Stream mode */
#define ADXL367_FIFO_MODE_TRIGGERED       0x3 /**< Triggered mode */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADI_ADX367_H_ */
