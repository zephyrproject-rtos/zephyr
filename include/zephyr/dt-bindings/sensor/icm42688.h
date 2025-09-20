/*
 * Copyright (c) 2024 Intel Corporation
 * Copyright (c) 2025 Croxel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for ICM42688 Devicetree constants
 * @ingroup icm42688_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_TDK_ICM42688P_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_TDK_ICM42688P_H_

#include "icm4268x.h"

/**
 * @brief Invensense (TDK) ICM42688 6-axis motion tracking device
 * @defgroup icm42688_interface ICM42688
 * @ingroup sensor_interface_ext
 * @{
 */

/**
 * @defgroup icm42688_accel_power_modes Accelerometer power modes
 * @{
 */
#define ICM42688_DT_ACCEL_OFF		ICM4268X_DT_ACCEL_OFF /**< Off */
#define ICM42688_DT_ACCEL_LP		ICM4268X_DT_ACCEL_LP  /**< Low Power */
#define ICM42688_DT_ACCEL_LN		ICM4268X_DT_ACCEL_LN  /**< Low Noise */
/** @} */

/**
 * @defgroup icm42688_gyro_power_modes Gyroscope power modes
 * @{
 */
#define ICM42688_DT_GYRO_OFF		ICM4268X_DT_GYRO_OFF      /**< Off */
#define ICM42688_DT_GYRO_STANDBY	ICM4268X_DT_GYRO_STANDBY /**< Standby */
#define ICM42688_DT_GYRO_LN		ICM4268X_DT_GYRO_LN      /**< Low Noise */
/** @} */

/**
 * @defgroup icm42688_accel_scale Accelerometer scale options
 * @{
 */
#define ICM42688_DT_ACCEL_FS_16 	0 /**< 16 g */
#define ICM42688_DT_ACCEL_FS_8		1 /**< 8 g */
#define ICM42688_DT_ACCEL_FS_4		2 /**< 4 g */
#define ICM42688_DT_ACCEL_FS_2		3 /**< 2 g */
/** @} */

/**
 * @defgroup icm42688_gyro_scale Gyroscope scale options
 * @{
 */
#define ICM42688_DT_GYRO_FS_2000		0 /**< 2000 dps */
#define ICM42688_DT_GYRO_FS_1000		1 /**< 1000 dps */
#define ICM42688_DT_GYRO_FS_500			2 /**< 500 dps */
#define ICM42688_DT_GYRO_FS_250			3 /**< 250 dps */
#define ICM42688_DT_GYRO_FS_125			4 /**< 125 dps */
#define ICM42688_DT_GYRO_FS_62_5		5 /**< 62.5 dps */
#define ICM42688_DT_GYRO_FS_31_25		6 /**< 31.25 dps */
#define ICM42688_DT_GYRO_FS_15_625		7 /**< 15.625 dps */
/** @} */

/**
 * @defgroup icm42688_accel_data_rate Accelerometer data rate options
 * @{
 */
#define ICM42688_DT_ACCEL_ODR_32000		ICM4268X_DT_ACCEL_ODR_32000     /**< 32 kHz */
#define ICM42688_DT_ACCEL_ODR_16000		ICM4268X_DT_ACCEL_ODR_16000     /**< 16 kHz */
#define ICM42688_DT_ACCEL_ODR_8000		ICM4268X_DT_ACCEL_ODR_8000      /**< 8 kHz */
#define ICM42688_DT_ACCEL_ODR_4000		ICM4268X_DT_ACCEL_ODR_4000      /**< 4 kHz */
#define ICM42688_DT_ACCEL_ODR_2000		ICM4268X_DT_ACCEL_ODR_2000      /**< 2 kHz */
#define ICM42688_DT_ACCEL_ODR_1000		ICM4268X_DT_ACCEL_ODR_1000      /**< 1 kHz */
#define ICM42688_DT_ACCEL_ODR_200		ICM4268X_DT_ACCEL_ODR_200       /**< 200 Hz */
#define ICM42688_DT_ACCEL_ODR_100		ICM4268X_DT_ACCEL_ODR_100       /**< 100 Hz */
#define ICM42688_DT_ACCEL_ODR_50		ICM4268X_DT_ACCEL_ODR_50        /**< 50 Hz */
#define ICM42688_DT_ACCEL_ODR_25		ICM4268X_DT_ACCEL_ODR_25        /**< 25 Hz */
#define ICM42688_DT_ACCEL_ODR_12_5		ICM4268X_DT_ACCEL_ODR_12_5      /**< 12.5 Hz */
#define ICM42688_DT_ACCEL_ODR_6_25		ICM4268X_DT_ACCEL_ODR_6_25      /**< 6.25 Hz */
#define ICM42688_DT_ACCEL_ODR_3_125		ICM4268X_DT_ACCEL_ODR_3_125     /**< 3.125 Hz */
#define ICM42688_DT_ACCEL_ODR_1_5625		ICM4268X_DT_ACCEL_ODR_1_5625    /**< 1.5625 Hz */
#define ICM42688_DT_ACCEL_ODR_500		ICM4268X_DT_ACCEL_ODR_500       /**< 500 Hz */
/** @} */

/**
 * @defgroup icm42688_gyro_data_rate Gyroscope data rate options
 * @{
 */
#define ICM42688_DT_GYRO_ODR_32000		ICM4268X_DT_GYRO_ODR_32000      /**< 32 kHz */
#define ICM42688_DT_GYRO_ODR_16000		ICM4268X_DT_GYRO_ODR_16000      /**< 16 kHz */
#define ICM42688_DT_GYRO_ODR_8000		ICM4268X_DT_GYRO_ODR_8000       /**< 8 kHz */
#define ICM42688_DT_GYRO_ODR_4000		ICM4268X_DT_GYRO_ODR_4000       /**< 4 kHz */
#define ICM42688_DT_GYRO_ODR_2000		ICM4268X_DT_GYRO_ODR_2000       /**< 2 kHz */
#define ICM42688_DT_GYRO_ODR_1000		ICM4268X_DT_GYRO_ODR_1000       /**< 1 kHz */
#define ICM42688_DT_GYRO_ODR_200		ICM4268X_DT_GYRO_ODR_200        /**< 200 Hz */
#define ICM42688_DT_GYRO_ODR_100		ICM4268X_DT_GYRO_ODR_100        /**< 100 Hz */
#define ICM42688_DT_GYRO_ODR_50			ICM4268X_DT_GYRO_ODR_50         /**< 50 Hz */
#define ICM42688_DT_GYRO_ODR_25			ICM4268X_DT_GYRO_ODR_25         /**< 25 Hz */
#define ICM42688_DT_GYRO_ODR_12_5		ICM4268X_DT_GYRO_ODR_12_5       /**< 12.5 Hz */
#define ICM42688_DT_GYRO_ODR_500		ICM4268X_DT_GYRO_ODR_500        /**< 500 Hz */
/** @} */

/** @} */

#endif /*ZEPHYR_INCLUDE_DT_BINDINGS_TDK_ICM42688P_H_ */
