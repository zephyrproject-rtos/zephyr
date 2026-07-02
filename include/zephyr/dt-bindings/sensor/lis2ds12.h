/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the ST LIS2DS12 accelerometer.
 * @ingroup lis2ds12_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ST_LIS2DS12_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ST_LIS2DS12_H_

/**
 * @defgroup lis2ds12_interface LIS2DS12
 * @ingroup sensor_interface_ext_st
 * @brief STMicroelectronics LIS2DS12 3-axis accelerometer
 * @{
 */

/**
 * @name Power modes
 *
 * Values for the `power-mode` devicetree property.
 * @{
 */
#define LIS2DS12_DT_POWER_DOWN			0 /**< Power-down */
#define LIS2DS12_DT_LOW_POWER			1 /**< Low-power mode */
#define LIS2DS12_DT_HIGH_RESOLUTION		2 /**< High-resolution mode */
#define LIS2DS12_DT_HIGH_FREQUENCY		3 /**< High-frequency mode */
/** @} */

/**
 * @name Output data rate options
 *
 * Values for the `odr` devicetree property.
 *
 * LP: low-power, HR: high-resolution, HF: high-frequency mode.
 * @{
 */
#define LIS2DS12_DT_ODR_OFF			0  /**< Power-down */
#define LIS2DS12_DT_ODR_1Hz_LP			1  /**< 1 Hz (LP mode only) */
#define LIS2DS12_DT_ODR_12Hz5			2  /**< 12.5 Hz (LP or HR mode) */
#define LIS2DS12_DT_ODR_25Hz			3  /**< 25 Hz (LP or HR mode) */
#define LIS2DS12_DT_ODR_50Hz			4  /**< 50 Hz (LP or HR mode) */
#define LIS2DS12_DT_ODR_100Hz			5  /**< 100 Hz (LP or HR mode) */
#define LIS2DS12_DT_ODR_200Hz			6  /**< 200 Hz (LP or HR mode) */
#define LIS2DS12_DT_ODR_400Hz			7  /**< 400 Hz (LP or HR mode) */
#define LIS2DS12_DT_ODR_800Hz			8  /**< 800 Hz (LP or HR mode) */
#define LIS2DS12_DT_ODR_1600Hz			9  /**< 1600 Hz (HF mode only) */
#define LIS2DS12_DT_ODR_3200Hz_HF		10 /**< 3200 Hz (HF mode only) */
#define LIS2DS12_DT_ODR_6400Hz_HF		11 /**< 6400 Hz (HF mode only) */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ST_LIS2DS12_H_ */
