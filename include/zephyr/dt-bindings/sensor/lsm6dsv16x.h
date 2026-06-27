/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the ST LSM6DSV16X IMU.
 * @ingroup lsm6dsv16x_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ST_LSM6DSV16X_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ST_LSM6DSV16X_H_

#include "lsm6dsvxxx.h"

/**
 * @defgroup lsm6dsv16x_interface LSM6DSV16X
 * @ingroup sensor_interface_ext_st
 * @brief STMicroelectronics LSM6DSV16X 6-axis IMU
 *
 * Output data rates and FIFO settings are shared across the family; see
 * @ref lsm6dsvxxx_interface.
 * @{
 */

/**
 * @name Accelerometer full-scale range
 *
 * Values for the `accel-range` devicetree property.
 * @{
 */
#define LSM6DSV16X_DT_FS_2G			2  /**< ±2 g */
#define LSM6DSV16X_DT_FS_4G			4  /**< ±4 g */
#define LSM6DSV16X_DT_FS_8G			8  /**< ±8 g */
#define LSM6DSV16X_DT_FS_16G			16 /**< ±16 g */
/** @} */

/**
 * @name Gyroscope full-scale range
 *
 * Values for the `gyro-range` devicetree property.
 * @{
 */
#define LSM6DSV16X_DT_FS_125DPS			0x0 /**< ±125 dps */
#define LSM6DSV16X_DT_FS_250DPS			0x1 /**< ±250 dps */
#define LSM6DSV16X_DT_FS_500DPS			0x2 /**< ±500 dps */
#define LSM6DSV16X_DT_FS_1000DPS		0x3 /**< ±1000 dps */
#define LSM6DSV16X_DT_FS_2000DPS		0x4 /**< ±2000 dps */
#define LSM6DSV16X_DT_FS_4000DPS		0xc /**< ±4000 dps */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ST_LSM6DSV16X_H_ */
