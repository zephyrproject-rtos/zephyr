/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the ST LSM6DSO IMU.
 * @ingroup lsm6dso_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ST_LSM6DSO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ST_LSM6DSO_H_

/**
 * @defgroup lsm6dso_interface LSM6DSO
 * @ingroup sensor_interface_ext_st
 * @brief STMicroelectronics LSM6DSO 6-axis IMU
 * @{
 */

/**
 * @name Accelerometer power modes
 *
 * Values for the `accel-pm` devicetree property.
 * @{
 */
#define LSM6DSO_DT_XL_HP_MODE			0 /**< High-performance mode */
#define LSM6DSO_DT_XL_LP_NORMAL_MODE		1 /**< Low-power / normal mode */
#define LSM6DSO_DT_XL_ULP_MODE			2 /**< Ultra-low-power mode */
/** @} */

/**
 * @name Gyroscope power modes
 *
 * Values for the `gyro-pm` devicetree property.
 * @{
 */
#define LSM6DSO_DT_GY_HP_MODE			0 /**< High-performance mode */
#define LSM6DSO_DT_GY_NORMAL_MODE		1 /**< Normal mode */
/** @} */

/**
 * @name Accelerometer full-scale range
 *
 * Values for the `accel-range` devicetree property.
 * @{
 */
#define LSM6DSO_DT_FS_2G			0 /**< ±2 g */
#define LSM6DSO_DT_FS_16G			1 /**< ±16 g */
#define LSM6DSO_DT_FS_4G			2 /**< ±4 g */
#define LSM6DSO_DT_FS_8G			3 /**< ±8 g */
/** @} */

/**
 * @name Gyroscope full-scale range
 *
 * Values for the `gyro-range` devicetree property.
 * @{
 */
#define LSM6DSO_DT_FS_250DPS			0 /**< ±250 dps */
#define LSM6DSO_DT_FS_125DPS			1 /**< ±125 dps */
#define LSM6DSO_DT_FS_500DPS			2 /**< ±500 dps */
#define LSM6DSO_DT_FS_1000DPS			4 /**< ±1000 dps */
#define LSM6DSO_DT_FS_2000DPS			6 /**< ±2000 dps */
/** @} */

/**
 * @name Accelerometer and gyroscope output data rate
 *
 * Values for the `accel-odr` and `gyro-odr` devicetree properties.
 * @{
 */
#define LSM6DSO_DT_ODR_OFF			0x0 /**< Power-down */
#define LSM6DSO_DT_ODR_12Hz5			0x1 /**< 12.5 Hz */
#define LSM6DSO_DT_ODR_26H			0x2 /**< 26 Hz */
#define LSM6DSO_DT_ODR_52Hz			0x3 /**< 52 Hz */
#define LSM6DSO_DT_ODR_104Hz			0x4 /**< 104 Hz */
#define LSM6DSO_DT_ODR_208Hz			0x5 /**< 208 Hz */
#define LSM6DSO_DT_ODR_417Hz			0x6 /**< 417 Hz */
#define LSM6DSO_DT_ODR_833Hz			0x7 /**< 833 Hz */
#define LSM6DSO_DT_ODR_1667Hz			0x8 /**< 1667 Hz */
#define LSM6DSO_DT_ODR_3333Hz			0x9 /**< 3333 Hz */
#define LSM6DSO_DT_ODR_6667Hz			0xa /**< 6667 Hz */
#define LSM6DSO_DT_ODR_1Hz6			0xb /**< 1.6 Hz (low-power) */
/** @} */

/**
 * @name Low-pass filter divider options
 *
 * Values for the `accel-lp-filter` devicetree property.
 * @{
 */
#define LSM6DSO_DT_LP_ODR_DIV_2			0 /**< ODR / 2 */
#define LSM6DSO_DT_LP_ODR_DIV_10		1 /**< ODR / 10 */
#define LSM6DSO_DT_LP_ODR_DIV_20		2 /**< ODR / 20 */
#define LSM6DSO_DT_LP_ODR_DIV_45		3 /**< ODR / 45 */
#define LSM6DSO_DT_LP_ODR_DIV_100		4 /**< ODR / 100 */
#define LSM6DSO_DT_LP_ODR_DIV_200		5 /**< ODR / 200 */
#define LSM6DSO_DT_LP_ODR_DIV_400		6 /**< ODR / 400 */
#define LSM6DSO_DT_LP_ODR_DIV_800		7 /**< ODR / 800 */
/** @} */

/**
 * @name Tap detection mode
 *
 * Values for the `tap-mode` devicetree property.
 * @{
 */
#define LSM6DSO_DT_SINGLE_TAP			0 /**< Single-tap only */
#define LSM6DSO_DT_SINGLE_DOUBLE_TAP		1 /**< Single and double tap */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ST_LSM6DSO_H_ */
