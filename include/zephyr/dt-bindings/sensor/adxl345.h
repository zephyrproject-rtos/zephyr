/*
 * Copyright (c) 2025 Croxel Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for ADXL345 Devicetree constants
 * @ingroup adxl345_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADI_ADX345_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADI_ADX345_H_

/**
 * @defgroup adxl345_interface ADXL345
 * @ingroup sensor_interface_ext
 * @brief Analog Devices ADXL345 3-axis accelerometer
 * @{
 */

/**
 * @defgroup adxl345_odr Output Data Rate options
 * @{
 */
#define ADXL345_DT_ODR_12_5		7    /**< 12.5 Hz */
#define ADXL345_DT_ODR_25		8    /**< 25 Hz */
#define ADXL345_DT_ODR_50		9    /**< 50 Hz */
#define ADXL345_DT_ODR_100		10   /**< 100 Hz */
#define ADXL345_DT_ODR_200		11   /**< 200 Hz */
#define ADXL345_DT_ODR_400		12   /**< 400 Hz */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADI_ADX345_H_ */
