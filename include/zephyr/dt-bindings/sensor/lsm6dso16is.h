/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the ST LSM6DSO16IS IMU.
 * @ingroup lsm6dso16is_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_LSM6DSO16IS_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_LSM6DSO16IS_H_

/**
 * @defgroup lsm6dso16is_interface LSM6DSO16IS
 * @ingroup sensor_interface_ext_st
 * @brief STMicroelectronics LSM6DSO16IS 6-axis IMU
 * @{
 */

/**
 * @name Accelerometer full-scale range
 *
 * Values for the `accel-range` devicetree property.
 * @{
 */
#define	LSM6DSO16IS_DT_FS_2G			0 /**< ±2 g */
#define	LSM6DSO16IS_DT_FS_16G			1 /**< ±16 g */
#define	LSM6DSO16IS_DT_FS_4G			2 /**< ±4 g */
#define	LSM6DSO16IS_DT_FS_8G			3 /**< ±8 g */
/** @} */

/**
 * @name Gyroscope full-scale range
 *
 * Values for the `gyro-range` devicetree property.
 * @{
 */
#define	LSM6DSO16IS_DT_FS_250DPS		0x0  /**< ±250 dps */
#define	LSM6DSO16IS_DT_FS_500DPS		0x1  /**< ±500 dps */
#define	LSM6DSO16IS_DT_FS_1000DPS		0x2  /**< ±1000 dps */
#define	LSM6DSO16IS_DT_FS_2000DPS		0x3  /**< ±2000 dps */
#define	LSM6DSO16IS_DT_FS_125DPS		0x10 /**< ±125 dps */
/** @} */

/**
 * @name Accelerometer and gyroscope output data rate
 *
 * Values for the `accel-odr` and `gyro-odr` devicetree properties.
 *
 * HP: high-performance, LP: low-power mode.
 * @{
 */
#define LSM6DSO16IS_DT_ODR_OFF			0x0  /**< Power-down */
#define LSM6DSO16IS_DT_ODR_12Hz5_HP		0x1  /**< 12.5 Hz (HP) */
#define LSM6DSO16IS_DT_ODR_26H_HP		0x2  /**< 26 Hz (HP) */
#define LSM6DSO16IS_DT_ODR_52Hz_HP		0x3  /**< 52 Hz (HP) */
#define LSM6DSO16IS_DT_ODR_104Hz_HP		0x4  /**< 104 Hz (HP) */
#define LSM6DSO16IS_DT_ODR_208Hz_HP		0x5  /**< 208 Hz (HP) */
#define LSM6DSO16IS_DT_ODR_416Hz_HP		0x6  /**< 416 Hz (HP) */
#define LSM6DSO16IS_DT_ODR_833Hz_HP		0x7  /**< 833 Hz (HP) */
#define LSM6DSO16IS_DT_ODR_1667Hz_HP		0x8  /**< 1667 Hz (HP) */
#define LSM6DSO16IS_DT_ODR_3333Hz_HP		0x9  /**< 3333 Hz (HP) */
#define LSM6DSO16IS_DT_ODR_6667Hz_HP		0xa  /**< 6667 Hz (HP) */
#define LSM6DSO16IS_DT_ODR_12Hz5_LP		0x11 /**< 12.5 Hz (LP) */
#define LSM6DSO16IS_DT_ODR_26H_LP		0x12 /**< 26 Hz (LP) */
#define LSM6DSO16IS_DT_ODR_52Hz_LP		0x13 /**< 52 Hz (LP) */
#define LSM6DSO16IS_DT_ODR_104Hz_LP		0x14 /**< 104 Hz (LP) */
#define LSM6DSO16IS_DT_ODR_208Hz_LP		0x15 /**< 208 Hz (LP) */
#define LSM6DSO16IS_DT_ODR_416Hz_LP		0x16 /**< 416 Hz (LP) */
#define LSM6DSO16IS_DT_ODR_833Hz_LP		0x17 /**< 833 Hz (LP) */
#define LSM6DSO16IS_DT_ODR_1667Hz_LP		0x18 /**< 1667 Hz (LP) */
#define LSM6DSO16IS_DT_ODR_3333Hz_LP		0x19 /**< 3333 Hz (LP) */
#define LSM6DSO16IS_DT_ODR_6667Hz_LP		0x1a /**< 6667 Hz (LP) */
#define LSM6DSO16IS_DT_ODR_1Hz6_LP		0x1b /**< 1.6 Hz (LP) */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_LSM6DSO16IS_H_ */
