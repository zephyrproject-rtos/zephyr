/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the Analog Devices ADXL313 accelerometer.
 * @ingroup adxl313_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ADXL313_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ADXL313_H_

/**
 * @defgroup adxl313_interface ADXL313
 * @ingroup sensor_interface_ext_adi
 * @brief Analog Devices ADXL313 3-axis accelerometer
 * @{
 */

/**
 * @name Output data rate options
 *
 * Values for the `odr` devicetree property (BW_RATE register).
 * @{
 */
#define ADXL313_DT_ODR_6_25  6  /**< 6.25 Hz */
#define ADXL313_DT_ODR_12_5  7  /**< 12.5 Hz */
#define ADXL313_DT_ODR_25    8  /**< 25 Hz */
#define ADXL313_DT_ODR_50    9  /**< 50 Hz */
#define ADXL313_DT_ODR_100   10 /**< 100 Hz */
#define ADXL313_DT_ODR_200   11 /**< 200 Hz */
#define ADXL313_DT_ODR_400   12 /**< 400 Hz */
#define ADXL313_DT_ODR_800   13 /**< 800 Hz */
#define ADXL313_DT_ODR_1600  14 /**< 1600 Hz */
#define ADXL313_DT_ODR_3200  15 /**< 3200 Hz */
/** @} */

/**
 * @name Measurement range options
 *
 * Values for the `range` devicetree property (DATA_FORMAT register).
 * @{
 */
#define ADXL313_DT_RANGE_0_5G 0 /**< ±0.5 g */
#define ADXL313_DT_RANGE_1G   1 /**< ±1 g */
#define ADXL313_DT_RANGE_2G   2 /**< ±2 g */
#define ADXL313_DT_RANGE_4G   3 /**< ±4 g */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ADXL313_H_ */
