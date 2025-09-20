/*
 * Copyright (c) 2025 Sentry Technologies ApS
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_TDK_IIM4623X_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_TDK_IIM4623X_H_

/**
 * @defgroup IIM4623x Invensense (TDK) IIM4623x DT Options
 * @ingroup sensor_interface
 * @{
 */

/**
 * @defgroup IIM46234_GYRO_BW Gyroscope Bandwidth options
 * @{
 */
#define IIM4623X_DT_GYRO_BW_4 0
#define IIM4623X_DT_GYRO_BW_5 1
#define IIM4623X_DT_GYRO_BW_6 2
#define IIM4623X_DT_GYRO_BW_7 3
/** @} */

/**
 * @defgroup IIM46234_ACCEL_BW Accelerometer Bandwidth options
 * @{
 */
#define IIM4623X_DT_ACCEL_BW_4 0
#define IIM4623X_DT_ACCEL_BW_5 1
#define IIM4623X_DT_ACCEL_BW_6 2
#define IIM4623X_DT_ACCEL_BW_7 3
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_TDK_IIM4623X_H_ */
