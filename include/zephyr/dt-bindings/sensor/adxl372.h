/*
 * Copyright (c) 2025 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for ADXL372 Devicetree constants
 * @ingroup adxl372_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADI_ADX372_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADI_ADX372_H_

/**
 * @defgroup adxl372_interface ADXL372
 * @ingroup sensor_interface_ext
 * @brief ADXL372 3-axis accelerometer
 * @{
 */

/**
 * @defgroup ADXL372_FIFO_MODE FIFO mode options
 * @{
 */

#define ADXL372_FIFO_MODE_BYPASSED        0x0 /**< FIFO disabled */
#define ADXL372_FIFO_MODE_STREAMED        0x1 /**< Stream mode */
#define ADXL372_FIFO_MODE_TRIGGERED       0x2 /**< Triggered mode */
#define ADXL372_FIFO_MODE_OLD_SAVED       0x3 /**< Oldest saved mode */

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADI_ADX372_H_ */
