/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_TDK_ICM42688P_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_TDK_ICM42688P_H_

/**
 * @defgroup ICM42688 Invensense (TDK) ICM42688 DT Options
 * @ingroup sensor_interface
 * @{
 */

/**
 * @defgroup ICM42688_ACCEL_POWER_MODES Accelerometer power modes
 * @{
 */
#define ICM42688_DT_ACCEL_OFF		0
#define ICM42688_DT_ACCEL_LP		2
#define ICM42688_DT_ACCEL_LN		3
/** @} */

/**
 * @defgroup ICM42688_GYRO_POWER_MODES Gyroscope power modes
 * @{
 */
#define ICM42688_DT_GYRO_OFF		0
#define ICM42688_DT_GYRO_STANDBY	1
#define ICM42688_DT_GYRO_LN		3
/** @} */

/**
 * @defgroup ICM42688_ACCEL_SCALE Accelerometer scale options
 * @{
 */
#define ICM42688_DT_ACCEL_FS_16	0
#define ICM42688_DT_ACCEL_FS_8		1
#define ICM42688_DT_ACCEL_FS_4		2
#define ICM42688_DT_ACCEL_FS_2		3
/** @} */

/**
 * @defgroup ICM42688_GYRO_SCALE Gyroscope scale options
 * @{
 */
#define ICM42688_DT_GYRO_FS_2000		0
#define ICM42688_DT_GYRO_FS_1000		1
#define ICM42688_DT_GYRO_FS_500		2
#define ICM42688_DT_GYRO_FS_250		3
#define ICM42688_DT_GYRO_FS_125		4
#define ICM42688_DT_GYRO_FS_62_5		5
#define ICM42688_DT_GYRO_FS_31_25		6
#define ICM42688_DT_GYRO_FS_15_625		7
/** @} */

/**
 * @defgroup ICM42688_ACCEL_DATA_RATE Accelerometer data rate options
 * @{
 */
#define ICM42688_DT_ACCEL_ODR_32000		1
#define ICM42688_DT_ACCEL_ODR_16000		2
#define ICM42688_DT_ACCEL_ODR_8000		3
#define ICM42688_DT_ACCEL_ODR_4000		4
#define ICM42688_DT_ACCEL_ODR_2000		5
#define ICM42688_DT_ACCEL_ODR_1000		6
#define ICM42688_DT_ACCEL_ODR_200		7
#define ICM42688_DT_ACCEL_ODR_100		8
#define ICM42688_DT_ACCEL_ODR_50		9
#define ICM42688_DT_ACCEL_ODR_25		10
#define ICM42688_DT_ACCEL_ODR_12_5		11
#define ICM42688_DT_ACCEL_ODR_6_25		12
#define ICM42688_DT_ACCEL_ODR_3_125		13
#define ICM42688_DT_ACCEL_ODR_1_5625		14
#define ICM42688_DT_ACCEL_ODR_500		15
/** @} */

/**
 * @defgroup ICM42688_GYRO_DATA_RATE Gyroscope data rate options
 * @{
 */
#define ICM42688_DT_GYRO_ODR_32000		1
#define ICM42688_DT_GYRO_ODR_16000		2
#define ICM42688_DT_GYRO_ODR_8000		3
#define ICM42688_DT_GYRO_ODR_4000		4
#define ICM42688_DT_GYRO_ODR_2000		5
#define ICM42688_DT_GYRO_ODR_1000		6
#define ICM42688_DT_GYRO_ODR_200		7
#define ICM42688_DT_GYRO_ODR_100		8
#define ICM42688_DT_GYRO_ODR_50		9
#define ICM42688_DT_GYRO_ODR_25		10
#define ICM42688_DT_GYRO_ODR_12_5		11
#define ICM42688_DT_GYRO_ODR_500		15
/** @} */

/** @} */

#endif /*ZEPHYR_INCLUDE_DT_BINDINGS_TDK_ICM42688P_H_ */
