/*
 * Copyright (c) 2024 Intel Corporation
 * Copyright (c) 2025 Croxel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants shared by the TDK InvenSense ICM4268x IMUs.
 * @ingroup icm4268x_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_TDK_ICM4268X_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_TDK_ICM4268X_H_

#include "sensor_axis_align.h"

/**
 * @defgroup icm4268x_interface ICM4268X
 * @ingroup sensor_interface_ext_tdk
 * @brief TDK InvenSense ICM4268x 6-axis IMU family
 * @{
 */

/**
 * @name Accelerometer power modes
 *
 * Values for the `accel-pwr-mode` devicetree property.
 * @{
 */
#define ICM4268X_DT_ACCEL_OFF		0 /**< Powered down */
#define ICM4268X_DT_ACCEL_LP		2 /**< Low-power mode */
#define ICM4268X_DT_ACCEL_LN		3 /**< Low-noise mode */
/** @} */

/**
 * @name Gyroscope power modes
 *
 * Values for the `gyro-pwr-mode` devicetree property.
 * @{
 */
#define ICM4268X_DT_GYRO_OFF		0 /**< Powered down */
#define ICM4268X_DT_GYRO_STANDBY	1 /**< Standby */
#define ICM4268X_DT_GYRO_LN		3 /**< Low-noise mode */
/** @} */

/**
 * @name Accelerometer data rate options
 *
 * Values for the `accel-odr` devicetree property.
 * @{
 */
#define ICM4268X_DT_ACCEL_ODR_32000		1  /**< 32 kHz */
#define ICM4268X_DT_ACCEL_ODR_16000		2  /**< 16 kHz */
#define ICM4268X_DT_ACCEL_ODR_8000		3  /**< 8 kHz */
#define ICM4268X_DT_ACCEL_ODR_4000		4  /**< 4 kHz */
#define ICM4268X_DT_ACCEL_ODR_2000		5  /**< 2 kHz */
#define ICM4268X_DT_ACCEL_ODR_1000		6  /**< 1 kHz */
#define ICM4268X_DT_ACCEL_ODR_200		7  /**< 200 Hz */
#define ICM4268X_DT_ACCEL_ODR_100		8  /**< 100 Hz */
#define ICM4268X_DT_ACCEL_ODR_50		9  /**< 50 Hz */
#define ICM4268X_DT_ACCEL_ODR_25		10 /**< 25 Hz */
#define ICM4268X_DT_ACCEL_ODR_12_5		11 /**< 12.5 Hz */
#define ICM4268X_DT_ACCEL_ODR_6_25		12 /**< 6.25 Hz */
#define ICM4268X_DT_ACCEL_ODR_3_125		13 /**< 3.125 Hz */
#define ICM4268X_DT_ACCEL_ODR_1_5625		14 /**< 1.5625 Hz */
#define ICM4268X_DT_ACCEL_ODR_500		15 /**< 500 Hz */
/** @} */

/**
 * @name Gyroscope data rate options
 *
 * Values for the `gyro-odr` devicetree property.
 * @{
 */
#define ICM4268X_DT_GYRO_ODR_32000		1  /**< 32 kHz */
#define ICM4268X_DT_GYRO_ODR_16000		2  /**< 16 kHz */
#define ICM4268X_DT_GYRO_ODR_8000		3  /**< 8 kHz */
#define ICM4268X_DT_GYRO_ODR_4000		4  /**< 4 kHz */
#define ICM4268X_DT_GYRO_ODR_2000		5  /**< 2 kHz */
#define ICM4268X_DT_GYRO_ODR_1000		6  /**< 1 kHz */
#define ICM4268X_DT_GYRO_ODR_200		7  /**< 200 Hz */
#define ICM4268X_DT_GYRO_ODR_100		8  /**< 100 Hz */
#define ICM4268X_DT_GYRO_ODR_50			9  /**< 50 Hz */
#define ICM4268X_DT_GYRO_ODR_25			10 /**< 25 Hz */
#define ICM4268X_DT_GYRO_ODR_12_5		11 /**< 12.5 Hz */
#define ICM4268X_DT_GYRO_ODR_500		15 /**< 500 Hz */
/** @} */

/** @} */

#endif /*ZEPHYR_INCLUDE_DT_BINDINGS_TDK_ICM4268X_H_ */
