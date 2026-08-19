/*
 * Copyright (c) 2024 Intel Corporation
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the TDK InvenSense ICM45686 IMU.
 * @ingroup icm45686_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ICM45686_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ICM45686_H_

/**
 * @defgroup icm45686_interface ICM45686
 * @ingroup sensor_interface_ext_tdk
 * @brief TDK InvenSense ICM45686 6-axis IMU
 * @{
 */

/**
 * @name Accelerometer power modes
 *
 * Values for the `accel-pwr-mode` devicetree property.
 * @{
 */
#define ICM45686_DT_ACCEL_OFF		0 /**< Powered down */
#define ICM45686_DT_ACCEL_LP		2 /**< Low-power mode */
#define ICM45686_DT_ACCEL_LN		3 /**< Low-noise mode */
/** @} */

/**
 * @name Gyroscope power modes
 *
 * Values for the `gyro-pwr-mode` devicetree property.
 * @{
 */
#define ICM45686_DT_GYRO_OFF		0 /**< Powered down */
#define ICM45686_DT_GYRO_STANDBY	1 /**< Standby */
#define ICM45686_DT_GYRO_LP		2 /**< Low-power mode */
#define ICM45686_DT_GYRO_LN		3 /**< Low-noise mode */
/** @} */

/**
 * @name Accelerometer full-scale range options
 *
 * Values for the `accel-fs` devicetree property.
 * @{
 */
#define ICM45686_DT_ACCEL_FS_32		0 /**< ±32 g */
#define ICM45686_DT_ACCEL_FS_16		1 /**< ±16 g */
#define ICM45686_DT_ACCEL_FS_8		2 /**< ±8 g */
#define ICM45686_DT_ACCEL_FS_4		3 /**< ±4 g */
#define ICM45686_DT_ACCEL_FS_2		4 /**< ±2 g */
/** @} */

/**
 * @name Gyroscope full-scale range options
 *
 * Values for the `gyro-fs` devicetree property.
 * @{
 */
#define ICM45686_DT_GYRO_FS_4000	0 /**< ±4000 dps */
#define ICM45686_DT_GYRO_FS_2000	1 /**< ±2000 dps */
#define ICM45686_DT_GYRO_FS_1000	2 /**< ±1000 dps */
#define ICM45686_DT_GYRO_FS_500		3 /**< ±500 dps */
#define ICM45686_DT_GYRO_FS_250		4 /**< ±250 dps */
#define ICM45686_DT_GYRO_FS_125		5 /**< ±125 dps */
#define ICM45686_DT_GYRO_FS_62_5	6 /**< ±62.5 dps */
#define ICM45686_DT_GYRO_FS_31_25	7 /**< ±31.25 dps */
#define ICM45686_DT_GYRO_FS_15_625	8 /**< ±15.625 dps */
/** @} */

/**
 * @name Accelerometer data rate options
 *
 * Values for the `accel-odr` devicetree property.
 * @{
 */
#define ICM45686_DT_ACCEL_ODR_6400	3  /**< 6400 Hz (LN mode only) */
#define ICM45686_DT_ACCEL_ODR_3200	4  /**< 3200 Hz (LN mode only) */
#define ICM45686_DT_ACCEL_ODR_1600	5  /**< 1600 Hz (LN mode only) */
#define ICM45686_DT_ACCEL_ODR_800	6  /**< 800 Hz (LN mode only) */
#define ICM45686_DT_ACCEL_ODR_400	7  /**< 400 Hz (LN or LP mode) */
#define ICM45686_DT_ACCEL_ODR_200	8  /**< 200 Hz (LN or LP mode) */
#define ICM45686_DT_ACCEL_ODR_100	9  /**< 100 Hz (LN or LP mode) */
#define ICM45686_DT_ACCEL_ODR_50	10 /**< 50 Hz (LN or LP mode) */
#define ICM45686_DT_ACCEL_ODR_25	11 /**< 25 Hz (LN or LP mode) */
#define ICM45686_DT_ACCEL_ODR_12_5	12 /**< 12.5 Hz (LN or LP mode) */
#define ICM45686_DT_ACCEL_ODR_6_25	13 /**< 6.25 Hz (LP mode only) */
#define ICM45686_DT_ACCEL_ODR_3_125	14 /**< 3.125 Hz (LP mode only) */
#define ICM45686_DT_ACCEL_ODR_1_5625	15 /**< 1.5625 Hz (LP mode only) */
/** @} */

/**
 * @name Gyroscope data rate options
 *
 * Values for the `gyro-odr` devicetree property.
 * @{
 */
#define ICM45686_DT_GYRO_ODR_6400	3  /**< 6400 Hz (LN mode only) */
#define ICM45686_DT_GYRO_ODR_3200	4  /**< 3200 Hz (LN mode only) */
#define ICM45686_DT_GYRO_ODR_1600	5  /**< 1600 Hz (LN mode only) */
#define ICM45686_DT_GYRO_ODR_800	6  /**< 800 Hz (LN mode only) */
#define ICM45686_DT_GYRO_ODR_400	7  /**< 400 Hz (LN or LP mode) */
#define ICM45686_DT_GYRO_ODR_200	8  /**< 200 Hz (LN or LP mode) */
#define ICM45686_DT_GYRO_ODR_100	9  /**< 100 Hz (LN or LP mode) */
#define ICM45686_DT_GYRO_ODR_50		10 /**< 50 Hz (LN or LP mode) */
#define ICM45686_DT_GYRO_ODR_25		11 /**< 25 Hz (LN or LP mode) */
#define ICM45686_DT_GYRO_ODR_12_5	12 /**< 12.5 Hz (LN or LP mode) */
#define ICM45686_DT_GYRO_ODR_6_25	13 /**< 6.25 Hz (LP mode only) */
#define ICM45686_DT_GYRO_ODR_3_125	14 /**< 3.125 Hz (LP mode only) */
#define ICM45686_DT_GYRO_ODR_1_5625	15 /**< 1.5625 Hz (LP mode only) */
/** @} */

/**
 * @name Gyroscope low-pass filter options
 *
 * Values for the `gyro-lpf` devicetree property.
 *
 * Bandwidth is expressed as a fraction of the selected output data rate.
 * @{
 */
#define ICM45686_DT_GYRO_LPF_BW_OFF	0 /**< Filter disabled */
#define ICM45686_DT_GYRO_LPF_BW_1_4	1 /**< ODR / 4 */
#define ICM45686_DT_GYRO_LPF_BW_1_8	2 /**< ODR / 8 */
#define ICM45686_DT_GYRO_LPF_BW_1_16	3 /**< ODR / 16 */
#define ICM45686_DT_GYRO_LPF_BW_1_32	4 /**< ODR / 32 */
#define ICM45686_DT_GYRO_LPF_BW_1_64	5 /**< ODR / 64 */
#define ICM45686_DT_GYRO_LPF_BW_1_128	6 /**< ODR / 128 */
/** @} */

/**
 * @name Accelerometer low-pass filter options
 *
 * Values for the `accel-lpf` devicetree property.
 *
 * Bandwidth is expressed as a fraction of the selected output data rate.
 * @{
 */
#define ICM45686_DT_ACCEL_LPF_BW_OFF	0 /**< Filter disabled */
#define ICM45686_DT_ACCEL_LPF_BW_1_4	1 /**< ODR / 4 */
#define ICM45686_DT_ACCEL_LPF_BW_1_8	2 /**< ODR / 8 */
#define ICM45686_DT_ACCEL_LPF_BW_1_16	3 /**< ODR / 16 */
#define ICM45686_DT_ACCEL_LPF_BW_1_32	4 /**< ODR / 32 */
#define ICM45686_DT_ACCEL_LPF_BW_1_64	5 /**< ODR / 64 */
#define ICM45686_DT_ACCEL_LPF_BW_1_128	6 /**< ODR / 128 */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ICM45686_H_ */
