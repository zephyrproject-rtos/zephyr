/*
 * Copyright (c) 2024 Intel Corporation
 * Copyright (c) 2025 Croxel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the TDK InvenSense ICM42686 IMU.
 * @ingroup icm42686_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ICM42686_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ICM42686_H_

#include "icm4268x.h"

/**
 * @defgroup icm42686_interface ICM42686
 * @ingroup sensor_interface_ext_tdk
 * @brief TDK InvenSense ICM42686 6-axis IMU
 * @{
 */

/**
 * @name Accelerometer power modes
 *
 * Values for the `accel-pwr-mode` devicetree property.
 * @{
 */
#define ICM42686_DT_ACCEL_OFF		ICM4268X_DT_ACCEL_OFF /**< Powered down */
#define ICM42686_DT_ACCEL_LP		ICM4268X_DT_ACCEL_LP  /**< Low-power mode */
#define ICM42686_DT_ACCEL_LN		ICM4268X_DT_ACCEL_LN  /**< Low-noise mode */
/** @} */

/**
 * @name Gyroscope power modes
 *
 * Values for the `gyro-pwr-mode` devicetree property.
 * @{
 */
#define ICM42686_DT_GYRO_OFF		ICM4268X_DT_GYRO_OFF     /**< Powered down */
#define ICM42686_DT_GYRO_STANDBY	ICM4268X_DT_GYRO_STANDBY /**< Standby */
#define ICM42686_DT_GYRO_LN		ICM4268X_DT_GYRO_LN      /**< Low-noise mode */
/** @} */

/**
 * @name Accelerometer full-scale range options
 *
 * Values for the `accel-fs` devicetree property.
 * @{
 */
#define ICM42686_DT_ACCEL_FS_32		0 /**< ±32 g */
#define ICM42686_DT_ACCEL_FS_16		1 /**< ±16 g */
#define ICM42686_DT_ACCEL_FS_8		2 /**< ±8 g */
#define ICM42686_DT_ACCEL_FS_4		3 /**< ±4 g */
#define ICM42686_DT_ACCEL_FS_2		4 /**< ±2 g */
/** @} */

/**
 * @name Gyroscope full-scale range options
 *
 * Values for the `gyro-fs` devicetree property.
 * @{
 */
#define ICM42686_DT_GYRO_FS_4000		0 /**< ±4000 dps */
#define ICM42686_DT_GYRO_FS_2000		1 /**< ±2000 dps */
#define ICM42686_DT_GYRO_FS_1000		2 /**< ±1000 dps */
#define ICM42686_DT_GYRO_FS_500			3 /**< ±500 dps */
#define ICM42686_DT_GYRO_FS_250			4 /**< ±250 dps */
#define ICM42686_DT_GYRO_FS_125			5 /**< ±125 dps */
#define ICM42686_DT_GYRO_FS_62_5		6 /**< ±62.5 dps */
#define ICM42686_DT_GYRO_FS_31_25		7 /**< ±31.25 dps */
/** @} */

/**
 * @name Accelerometer data rate options
 *
 * Values for the `accel-odr` devicetree property.
 * @{
 */
#define ICM42686_DT_ACCEL_ODR_32000		ICM4268X_DT_ACCEL_ODR_32000 /**< 32 kHz */
#define ICM42686_DT_ACCEL_ODR_16000		ICM4268X_DT_ACCEL_ODR_16000 /**< 16 kHz */
#define ICM42686_DT_ACCEL_ODR_8000		ICM4268X_DT_ACCEL_ODR_8000  /**< 8 kHz */
#define ICM42686_DT_ACCEL_ODR_4000		ICM4268X_DT_ACCEL_ODR_4000  /**< 4 kHz */
#define ICM42686_DT_ACCEL_ODR_2000		ICM4268X_DT_ACCEL_ODR_2000  /**< 2 kHz */
#define ICM42686_DT_ACCEL_ODR_1000		ICM4268X_DT_ACCEL_ODR_1000  /**< 1 kHz */
#define ICM42686_DT_ACCEL_ODR_200		ICM4268X_DT_ACCEL_ODR_200   /**< 200 Hz */
#define ICM42686_DT_ACCEL_ODR_100		ICM4268X_DT_ACCEL_ODR_100   /**< 100 Hz */
#define ICM42686_DT_ACCEL_ODR_50		ICM4268X_DT_ACCEL_ODR_50    /**< 50 Hz */
#define ICM42686_DT_ACCEL_ODR_25		ICM4268X_DT_ACCEL_ODR_25    /**< 25 Hz */
#define ICM42686_DT_ACCEL_ODR_12_5		ICM4268X_DT_ACCEL_ODR_12_5  /**< 12.5 Hz */
#define ICM42686_DT_ACCEL_ODR_6_25		ICM4268X_DT_ACCEL_ODR_6_25  /**< 6.25 Hz */
#define ICM42686_DT_ACCEL_ODR_3_125		ICM4268X_DT_ACCEL_ODR_3_125 /**< 3.125 Hz */
#define ICM42686_DT_ACCEL_ODR_1_5625		ICM4268X_DT_ACCEL_ODR_1_5625 /**< 1.5625 Hz */
#define ICM42686_DT_ACCEL_ODR_500		ICM4268X_DT_ACCEL_ODR_500   /**< 500 Hz */
/** @} */

/**
 * @name Gyroscope data rate options
 *
 * Values for the `gyro-odr` devicetree property.
 * @{
 */
#define ICM42686_DT_GYRO_ODR_32000		ICM4268X_DT_GYRO_ODR_32000 /**< 32 kHz */
#define ICM42686_DT_GYRO_ODR_16000		ICM4268X_DT_GYRO_ODR_16000 /**< 16 kHz */
#define ICM42686_DT_GYRO_ODR_8000		ICM4268X_DT_GYRO_ODR_8000  /**< 8 kHz */
#define ICM42686_DT_GYRO_ODR_4000		ICM4268X_DT_GYRO_ODR_4000  /**< 4 kHz */
#define ICM42686_DT_GYRO_ODR_2000		ICM4268X_DT_GYRO_ODR_2000  /**< 2 kHz */
#define ICM42686_DT_GYRO_ODR_1000		ICM4268X_DT_GYRO_ODR_1000  /**< 1 kHz */
#define ICM42686_DT_GYRO_ODR_200		ICM4268X_DT_GYRO_ODR_200   /**< 200 Hz */
#define ICM42686_DT_GYRO_ODR_100		ICM4268X_DT_GYRO_ODR_100   /**< 100 Hz */
#define ICM42686_DT_GYRO_ODR_50			ICM4268X_DT_GYRO_ODR_50    /**< 50 Hz */
#define ICM42686_DT_GYRO_ODR_25			ICM4268X_DT_GYRO_ODR_25    /**< 25 Hz */
#define ICM42686_DT_GYRO_ODR_12_5		ICM4268X_DT_GYRO_ODR_12_5  /**< 12.5 Hz */
#define ICM42686_DT_GYRO_ODR_500		ICM4268X_DT_GYRO_ODR_500   /**< 500 Hz */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ICM42686_H_ */
