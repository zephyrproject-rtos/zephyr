/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the ST LSM6DSV80X IMU.
 * @ingroup lsm6dsv80x_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_LSM6DSV80X_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_LSM6DSV80X_H_

#include "lsm6dsvxxx.h"

/**
 * @defgroup lsm6dsv80x_interface LSM6DSV80X
 * @ingroup sensor_interface_ext_st
 * @brief STMicroelectronics LSM6DSV80X dual accelerometer and gyroscope IMU
 *
 * Output data rates and FIFO settings are shared across the family; see
 * @ref lsm6dsvxxx_interface.
 * @{
 */

/**
 * @name High-g accelerometer output data rate
 *
 * Values for the `accel-hg-odr` devicetree property.
 * @{
 */
#define LSM6DSV80X_HG_XL_ODR_OFF		0x0 /**< Power-down */
#define LSM6DSV80X_HG_XL_ODR_AT_480Hz		0x3 /**< 480 Hz */
#define LSM6DSV80X_HG_XL_ODR_AT_960Hz		0x4 /**< 960 Hz */
#define LSM6DSV80X_HG_XL_ODR_AT_1920Hz		0x5 /**< 1920 Hz */
#define LSM6DSV80X_HG_XL_ODR_AT_3840Hz		0x6 /**< 3840 Hz */
#define LSM6DSV80X_HG_XL_ODR_AT_7680Hz		0x7 /**< 7680 Hz */
/** @} */

/**
 * @name Accelerometer full-scale range
 *
 * Values for the `accel-range` devicetree property.
 * @{
 */
#define LSM6DSV80X_DT_FS_2G			2  /**< ±2 g */
#define LSM6DSV80X_DT_FS_4G			4  /**< ±4 g */
#define LSM6DSV80X_DT_FS_8G			8  /**< ±8 g */
#define LSM6DSV80X_DT_FS_16G			16 /**< ±16 g */
#define LSM6DSV80X_DT_FS_32G			32 /**< ±32 g */
#define LSM6DSV80X_DT_FS_64G			64 /**< ±64 g */
#define LSM6DSV80X_DT_FS_80G			80 /**< ±80 g */
/** @} */

/**
 * @name Gyroscope full-scale range
 *
 * Values for the `gyro-range` devicetree property.
 * @{
 */
#define LSM6DSV80X_DT_FS_250DPS			0x1 /**< ±250 dps */
#define LSM6DSV80X_DT_FS_500DPS			0x2 /**< ±500 dps */
#define LSM6DSV80X_DT_FS_1000DPS		0x3 /**< ±1000 dps */
#define LSM6DSV80X_DT_FS_2000DPS		0x4 /**< ±2000 dps */
#define LSM6DSV80X_DT_FS_4000DPS		0x5 /**< ±4000 dps */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_LSM6DSV80X_H_ */
