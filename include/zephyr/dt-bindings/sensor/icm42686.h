/*
 * Copyright (c) 2024 Intel Corporation
 * Copyright (c) 2025 Croxel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for ICM42686 Devicetree constants
 * @ingroup icm42686_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_TDK_ICM42686P_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_TDK_ICM42686P_H_

#include "icm4268x.h"

/**
 * @brief Invensense (TDK) ICM42686 6-axis motion tracking device
 * @defgroup icm42686_interface ICM42686
 * @ingroup sensor_interface_ext
 * @{
 */

/**
 * @defgroup icm42686_accel_power_modes Accelerometer power modes
 * @{
 */
#define ICM42686_DT_ACCEL_OFF		ICM4268X_DT_ACCEL_OFF /**< Off */
#define ICM42686_DT_ACCEL_LP		ICM4268X_DT_ACCEL_LP  /**< Low Power */
#define ICM42686_DT_ACCEL_LN		ICM4268X_DT_ACCEL_LN  /**< Low Noise */
/** @} */

/**
 * @defgroup icm42686_gyro_power_modes Gyroscope power modes
 * @{
 */
#define ICM42686_DT_GYRO_OFF		ICM4268X_DT_GYRO_OFF      /**< Off */
#define ICM42686_DT_GYRO_STANDBY	ICM4268X_DT_GYRO_STANDBY /**< Standby */
#define ICM42686_DT_GYRO_LN		ICM4268X_DT_GYRO_LN      /**< Low Noise */
/** @} */

/**
 * @defgroup ICM42686_ACCEL_SCALE Accelerometer scale options
 * @{
 */
#define ICM42686_DT_ACCEL_FS_32		0 /**< 32 g */
#define ICM42686_DT_ACCEL_FS_16		1 /**< 16 g */
#define ICM42686_DT_ACCEL_FS_8		2 /**< 8 g */
#define ICM42686_DT_ACCEL_FS_4		3 /**< 4 g */
#define ICM42686_DT_ACCEL_FS_2		4 /**< 2 g */
/** @} */

/**
 * @defgroup ICM42686_GYRO_SCALE Gyroscope scale options
 * @{
 */
#define ICM42686_DT_GYRO_FS_4000		0 /**< 4000 dps */
#define ICM42686_DT_GYRO_FS_2000		1 /**< 2000 dps */
#define ICM42686_DT_GYRO_FS_1000		2 /**< 1000 dps */
#define ICM42686_DT_GYRO_FS_500			3 /**< 500 dps */
#define ICM42686_DT_GYRO_FS_250			4 /**< 250 dps */
#define ICM42686_DT_GYRO_FS_125			5 /**< 125 dps */
#define ICM42686_DT_GYRO_FS_62_5		6 /**< 62.5 dps */
#define ICM42686_DT_GYRO_FS_31_25		7 /**< 31.25 dps */
/** @} */

/**
 * @defgroup ICM42686_ACCEL_DATA_RATE Accelerometer data rate options
 * @{
 */
#define ICM42686_DT_ACCEL_ODR_32000		ICM4268X_DT_ACCEL_ODR_32000     /**< 32 kHz */
#define ICM42686_DT_ACCEL_ODR_16000		ICM4268X_DT_ACCEL_ODR_16000     /**< 16 kHz */
#define ICM42686_DT_ACCEL_ODR_8000		ICM4268X_DT_ACCEL_ODR_8000      /**< 8 kHz */
#define ICM42686_DT_ACCEL_ODR_4000		ICM4268X_DT_ACCEL_ODR_4000      /**< 4 kHz */
#define ICM42686_DT_ACCEL_ODR_2000		ICM4268X_DT_ACCEL_ODR_2000      /**< 2 kHz */
#define ICM42686_DT_ACCEL_ODR_1000		ICM4268X_DT_ACCEL_ODR_1000      /**< 1 kHz */
#define ICM42686_DT_ACCEL_ODR_200		ICM4268X_DT_ACCEL_ODR_200       /**< 200 Hz */
#define ICM42686_DT_ACCEL_ODR_100		ICM4268X_DT_ACCEL_ODR_100       /**< 100 Hz */
#define ICM42686_DT_ACCEL_ODR_50		ICM4268X_DT_ACCEL_ODR_50        /**< 50 Hz */
#define ICM42686_DT_ACCEL_ODR_25		ICM4268X_DT_ACCEL_ODR_25        /**< 25 Hz */
#define ICM42686_DT_ACCEL_ODR_12_5		ICM4268X_DT_ACCEL_ODR_12_5      /**< 12.5 Hz */
#define ICM42686_DT_ACCEL_ODR_6_25		ICM4268X_DT_ACCEL_ODR_6_25      /**< 6.25 Hz */
#define ICM42686_DT_ACCEL_ODR_3_125		ICM4268X_DT_ACCEL_ODR_3_125     /**< 3.125 Hz */
#define ICM42686_DT_ACCEL_ODR_1_5625		ICM4268X_DT_ACCEL_ODR_1_5625    /**< 1.5625 Hz */
#define ICM42686_DT_ACCEL_ODR_500		ICM4268X_DT_ACCEL_ODR_500       /**< 500 Hz */
/** @} */

/**
 * @defgroup icm42686_gyro_data_rate Gyroscope data rate options
 * @{
 */
#define ICM42686_DT_GYRO_ODR_32000		ICM4268X_DT_GYRO_ODR_32000      /**< 32 kHz */
#define ICM42686_DT_GYRO_ODR_16000		ICM4268X_DT_GYRO_ODR_16000      /**< 16 kHz */
#define ICM42686_DT_GYRO_ODR_8000		ICM4268X_DT_GYRO_ODR_8000       /**< 8 kHz */
#define ICM42686_DT_GYRO_ODR_4000		ICM4268X_DT_GYRO_ODR_4000       /**< 4 kHz */
#define ICM42686_DT_GYRO_ODR_2000		ICM4268X_DT_GYRO_ODR_2000       /**< 2 kHz */
#define ICM42686_DT_GYRO_ODR_1000		ICM4268X_DT_GYRO_ODR_1000       /**< 1 kHz */
#define ICM42686_DT_GYRO_ODR_200		ICM4268X_DT_GYRO_ODR_200        /**< 200 Hz */
#define ICM42686_DT_GYRO_ODR_100		ICM4268X_DT_GYRO_ODR_100        /**< 100 Hz */
#define ICM42686_DT_GYRO_ODR_50			ICM4268X_DT_GYRO_ODR_50         /**< 50 Hz */
#define ICM42686_DT_GYRO_ODR_25			ICM4268X_DT_GYRO_ODR_25         /**< 25 Hz */
#define ICM42686_DT_GYRO_ODR_12_5		ICM4268X_DT_GYRO_ODR_12_5       /**< 12.5 Hz */
#define ICM42686_DT_GYRO_ODR_500		ICM4268X_DT_GYRO_ODR_500        /**< 500 Hz */
/** @} */

/** @} */

#endif /*ZEPHYR_INCLUDE_DT_BINDINGS_TDK_ICM42686P_H_ */
