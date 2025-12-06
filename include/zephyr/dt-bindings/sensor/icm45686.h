/*
 * Copyright (c) 2024 Intel Corporation
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for ICM45686 Devicetree constants
 * @ingroup icm45686_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_TDK_ICM45686_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_TDK_ICM45686_H_

/**
 * @brief Invensense (TDK) ICM45686 6-axis motion tracking device
 * @defgroup icm45686_interface ICM45686
 * @ingroup sensor_interface
 * @{
 */

/**
 * @defgroup icm45686_accel_power_modes Accelerometer power modes
 * @{
 */
#define ICM45686_DT_ACCEL_OFF		0
#define ICM45686_DT_ACCEL_LP		2
#define ICM45686_DT_ACCEL_LN		3
/** @} */

/**
 * @defgroup icm45686_gyro_power_modes Gyroscope power modes
 * @{
 */
#define ICM45686_DT_GYRO_OFF		0
#define ICM45686_DT_GYRO_STANDBY	1
#define ICM45686_DT_GYRO_LP		2
#define ICM45686_DT_GYRO_LN		3
/** @} */

/**
 * @defgroup icm45686_accel_scale Accelerometer scale options
 * @{
 */
#define ICM45686_DT_ACCEL_FS_32		0
#define ICM45686_DT_ACCEL_FS_16		1
#define ICM45686_DT_ACCEL_FS_8		2
#define ICM45686_DT_ACCEL_FS_4		3
#define ICM45686_DT_ACCEL_FS_2		4
/** @} */

/**
 * @defgroup icm45686_gyro_scale Gyroscope scale options
 * @{
 */
#define ICM45686_DT_GYRO_FS_4000	0
#define ICM45686_DT_GYRO_FS_2000	1
#define ICM45686_DT_GYRO_FS_1000	2
#define ICM45686_DT_GYRO_FS_500		3
#define ICM45686_DT_GYRO_FS_250		4
#define ICM45686_DT_GYRO_FS_125		5
#define ICM45686_DT_GYRO_FS_62_5	6
#define ICM45686_DT_GYRO_FS_31_25	7
#define ICM45686_DT_GYRO_FS_15_625	8
/** @} */

/**
 * @defgroup icm45686_accel_data_rate Accelerometer data rate options
 * @{
 */
#define ICM45686_DT_ACCEL_ODR_6400	3 /* LN-mode only */
#define ICM45686_DT_ACCEL_ODR_3200	4 /* LN-mode only */
#define ICM45686_DT_ACCEL_ODR_1600	5 /* LN-mode only */
#define ICM45686_DT_ACCEL_ODR_800	6 /* LN-mode only */
#define ICM45686_DT_ACCEL_ODR_400	7 /* Both LN-mode and LP-mode */
#define ICM45686_DT_ACCEL_ODR_200	8 /* Both LN-mode and LP-mode */
#define ICM45686_DT_ACCEL_ODR_100	9 /* Both LN-mode and LP-mode */
#define ICM45686_DT_ACCEL_ODR_50	10 /* Both LN-mode and LP-mode */
#define ICM45686_DT_ACCEL_ODR_25	11 /* Both LN-mode and LP-mode */
#define ICM45686_DT_ACCEL_ODR_12_5	12 /* Both LN-mode and LP-mode */
#define ICM45686_DT_ACCEL_ODR_6_25	13 /* LP-mode only */
#define ICM45686_DT_ACCEL_ODR_3_125	14 /* LP-mode only */
#define ICM45686_DT_ACCEL_ODR_1_5625	15 /* LP-mode only */
/** @} */

/**
 * @defgroup icm45686_gyro_data_rate Gyroscope data rate options
 * @{
 */
#define ICM45686_DT_GYRO_ODR_6400	3 /* LN-mode only */
#define ICM45686_DT_GYRO_ODR_3200	4 /* LN-mode only */
#define ICM45686_DT_GYRO_ODR_1600	5 /* LN-mode only */
#define ICM45686_DT_GYRO_ODR_800	6 /* LN-mode only */
#define ICM45686_DT_GYRO_ODR_400	7 /* Both LN-mode and LP-mode */
#define ICM45686_DT_GYRO_ODR_200	8 /* Both LN-mode and LP-mode */
#define ICM45686_DT_GYRO_ODR_100	9 /* Both LN-mode and LP-mode */
#define ICM45686_DT_GYRO_ODR_50		10 /* Both LN-mode and LP-mode */
#define ICM45686_DT_GYRO_ODR_25		11 /* Both LN-mode and LP-mode */
#define ICM45686_DT_GYRO_ODR_12_5	12 /* Both LN-mode and LP-mode */
#define ICM45686_DT_GYRO_ODR_6_25	13 /* LP-mode only */
#define ICM45686_DT_GYRO_ODR_3_125	14 /* LP-mode only */
#define ICM45686_DT_GYRO_ODR_1_5625	15 /* LP-mode only */
/** @} */

/**
 * @defgroup icm45686_gyro_lpf Gyroscope Low-pass Filtering options
 * @{
 */
#define ICM45686_DT_GYRO_LPF_BW_OFF	0
#define ICM45686_DT_GYRO_LPF_BW_1_4	1
#define ICM45686_DT_GYRO_LPF_BW_1_8	2
#define ICM45686_DT_GYRO_LPF_BW_1_16	3
#define ICM45686_DT_GYRO_LPF_BW_1_32	4
#define ICM45686_DT_GYRO_LPF_BW_1_64	5
#define ICM45686_DT_GYRO_LPF_BW_1_128	6
/** @} */

/**
 * @defgroup icm45686_accel_lpf Accelerometer Low-pass Filtering options
 * @{
 */
#define ICM45686_DT_ACCEL_LPF_BW_OFF	0
#define ICM45686_DT_ACCEL_LPF_BW_1_4	1
#define ICM45686_DT_ACCEL_LPF_BW_1_8	2
#define ICM45686_DT_ACCEL_LPF_BW_1_16	3
#define ICM45686_DT_ACCEL_LPF_BW_1_32	4
#define ICM45686_DT_ACCEL_LPF_BW_1_64	5
#define ICM45686_DT_ACCEL_LPF_BW_1_128	6
/** @} */


/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_TDK_ICM45686_H_ */
