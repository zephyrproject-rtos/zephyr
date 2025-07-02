/*
 * Copyright (c) 2024 Intel Corporation
 * Copyright (c) 2025 Croxel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_TDK_ICM4268X_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_TDK_ICM4268X_H_

#include "sensor_axis_align.h"

/**
 * @defgroup ICM4268X Invensense (TDK) ICM4268X DT Options
 * @ingroup sensor_interface
 * @{
 */

/**
 * @defgroup ICM4268X_ACCEL_POWER_MODES Accelerometer power modes
 * @{
 */
#define ICM4268X_DT_ACCEL_OFF		0
#define ICM4268X_DT_ACCEL_LP		2
#define ICM4268X_DT_ACCEL_LN		3
/** @} */

/**
 * @defgroup ICM4268X_GYRO_POWER_MODES Gyroscope power modes
 * @{
 */
#define ICM4268X_DT_GYRO_OFF		0
#define ICM4268X_DT_GYRO_STANDBY	1
#define ICM4268X_DT_GYRO_LN		3
/** @} */

/**
 * @defgroup ICM4268X_ACCEL_DATA_RATE Accelerometer data rate options
 * @{
 */
#define ICM4268X_DT_ACCEL_ODR_32000		1
#define ICM4268X_DT_ACCEL_ODR_16000		2
#define ICM4268X_DT_ACCEL_ODR_8000		3
#define ICM4268X_DT_ACCEL_ODR_4000		4
#define ICM4268X_DT_ACCEL_ODR_2000		5
#define ICM4268X_DT_ACCEL_ODR_1000		6
#define ICM4268X_DT_ACCEL_ODR_200		7
#define ICM4268X_DT_ACCEL_ODR_100		8
#define ICM4268X_DT_ACCEL_ODR_50		9
#define ICM4268X_DT_ACCEL_ODR_25		10
#define ICM4268X_DT_ACCEL_ODR_12_5		11
#define ICM4268X_DT_ACCEL_ODR_6_25		12
#define ICM4268X_DT_ACCEL_ODR_3_125		13
#define ICM4268X_DT_ACCEL_ODR_1_5625		14
#define ICM4268X_DT_ACCEL_ODR_500		15
/** @} */

/**
 * @defgroup ICM4268X_GYRO_DATA_RATE Gyroscope data rate options
 * @{
 */
#define ICM4268X_DT_GYRO_ODR_32000		1
#define ICM4268X_DT_GYRO_ODR_16000		2
#define ICM4268X_DT_GYRO_ODR_8000		3
#define ICM4268X_DT_GYRO_ODR_4000		4
#define ICM4268X_DT_GYRO_ODR_2000		5
#define ICM4268X_DT_GYRO_ODR_1000		6
#define ICM4268X_DT_GYRO_ODR_200		7
#define ICM4268X_DT_GYRO_ODR_100		8
#define ICM4268X_DT_GYRO_ODR_50			9
#define ICM4268X_DT_GYRO_ODR_25			10
#define ICM4268X_DT_GYRO_ODR_12_5		11
#define ICM4268X_DT_GYRO_ODR_500		15
/** @} */

/** @} */

#endif /*ZEPHYR_INCLUDE_DT_BINDINGS_TDK_ICM4268X_H_ */
