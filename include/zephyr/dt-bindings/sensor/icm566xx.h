/*
 * Copyright (c) 2024 Intel Corporation
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the TDK InvenSense ICM566xx IMU.
 * @ingroup icm566xx_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_TDK_ICM566XX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_TDK_ICM566XX_H_

/**
 * @defgroup icm566xx_interface ICM566xx
 * @ingroup sensor_interface_ext_tdk
 * @brief TDK InvenSense ICM566xx 6-axis IMU
 * @{
 */

/**
 * @name Accelerometer power modes
 *
 * Values for the `accel-pwr-mode` devicetree property.
 * @{
 */
#define ICM566XX_DT_ACCEL_OFF		0 /**< Powered down */
#define ICM566XX_DT_ACCEL_LP		2 /**< Low-power mode */
#define ICM566XX_DT_ACCEL_LN		3 /**< Low-noise mode */
/** @} */

/**
 * @name Gyroscope power modes
 *
 * Values for the `gyro-pwr-mode` devicetree property.
 * @{
 */
#define ICM566XX_DT_GYRO_OFF		0 /**< Powered down */
#define ICM566XX_DT_GYRO_STANDBY	1 /**< Standby */
#define ICM566XX_DT_GYRO_LP		2 /**< Low-power mode */
#define ICM566XX_DT_GYRO_LN		3 /**< Low-noise mode */
/** @} */

/**
 * @name Accelerometer scale options
 *
 * Values for the `accel-fs` devicetree property.
 * @{
 */
#define ICM566XX_DT_ACCEL_FS_32		0 /**< 32g full scale */
#define ICM566XX_DT_ACCEL_FS_16		1 /**< 16g full scale */
#define ICM566XX_DT_ACCEL_FS_8		2 /**< 8g full scale */
#define ICM566XX_DT_ACCEL_FS_4		3 /**< 4g full scale */
#define ICM566XX_DT_ACCEL_FS_2		4 /**< 2g full scale */
/** @} */

/**
 * @name Gyroscope scale options
 *
 * Values for the `gyro-fs` devicetree property.
 * @{
 */
#define ICM566XX_DT_GYRO_FS_4000	0 /**< 4000 dps full scale */
#define ICM566XX_DT_GYRO_FS_2000	1 /**< 2000 dps full scale */
#define ICM566XX_DT_GYRO_FS_1000	2 /**< 1000 dps full scale */
#define ICM566XX_DT_GYRO_FS_500		3 /**< 500 dps full scale */
#define ICM566XX_DT_GYRO_FS_250		4 /**< 250 dps full scale */
#define ICM566XX_DT_GYRO_FS_125		5 /**< 125 dps full scale */
#define ICM566XX_DT_GYRO_FS_62_5	6 /**< 62.5 dps full scale */
#define ICM566XX_DT_GYRO_FS_31_25	7 /**< 31.25 dps full scale */
#define ICM566XX_DT_GYRO_FS_15_625	8 /**< 15.625 dps full scale */
/** @} */

/**
 * @name Accelerometer data rate options
 *
 * Values for the `accel-odr` devicetree property.
 * @{
 */
#define ICM566XX_DT_ACCEL_ODR_6400	3  /**< 6400 Hz (LN mode only) */
#define ICM566XX_DT_ACCEL_ODR_3200	4  /**< 3200 Hz (LN mode only) */
#define ICM566XX_DT_ACCEL_ODR_1600	5  /**< 1600 Hz (LN mode only) */
#define ICM566XX_DT_ACCEL_ODR_800	6  /**< 800 Hz (LN mode only) */
#define ICM566XX_DT_ACCEL_ODR_400	7  /**< 400 Hz (LN or LP mode) */
#define ICM566XX_DT_ACCEL_ODR_200	8  /**< 200 Hz (LN or LP mode) */
#define ICM566XX_DT_ACCEL_ODR_100	9  /**< 100 Hz (LN or LP mode) */
#define ICM566XX_DT_ACCEL_ODR_50	10 /**< 50 Hz (LN or LP mode) */
#define ICM566XX_DT_ACCEL_ODR_25	11 /**< 25 Hz (LN or LP mode) */
#define ICM566XX_DT_ACCEL_ODR_12_5	12 /**< 12.5 Hz (LN or LP mode) */
#define ICM566XX_DT_ACCEL_ODR_6_25	13 /**< 6.25 Hz (LP mode only) */
#define ICM566XX_DT_ACCEL_ODR_3_125	14 /**< 3.125 Hz (LP mode only) */
#define ICM566XX_DT_ACCEL_ODR_1_5625	15 /**< 1.5625 Hz (LP mode only) */
/** @} */

/**
 * @name Gyroscope data rate options
 *
 * Values for the `gyro-odr` devicetree property.
 * @{
 */
#define ICM566XX_DT_GYRO_ODR_6400	3  /**< 6400 Hz (LN mode only) */
#define ICM566XX_DT_GYRO_ODR_3200	4  /**< 3200 Hz (LN mode only) */
#define ICM566XX_DT_GYRO_ODR_1600	5  /**< 1600 Hz (LN mode only) */
#define ICM566XX_DT_GYRO_ODR_800	6  /**< 800 Hz (LN mode only) */
#define ICM566XX_DT_GYRO_ODR_400	7  /**< 400 Hz (LN or LP mode) */
#define ICM566XX_DT_GYRO_ODR_200	8  /**< 200 Hz (LN or LP mode) */
#define ICM566XX_DT_GYRO_ODR_100	9  /**< 100 Hz (LN or LP mode) */
#define ICM566XX_DT_GYRO_ODR_50		10 /**< 50 Hz (LN or LP mode) */
#define ICM566XX_DT_GYRO_ODR_25		11 /**< 25 Hz (LN or LP mode) */
#define ICM566XX_DT_GYRO_ODR_12_5	1  /**< 12.5 Hz (LN or LP mode) */
#define ICM566XX_DT_GYRO_ODR_6_25	13 /**< 6.25 Hz (LP mode only) */
#define ICM566XX_DT_GYRO_ODR_3_125	14 /**< 3.125 Hz (LP mode only) */
#define ICM566XX_DT_GYRO_ODR_1_5625	15 /**< 1.5625 Hz (LP mode only) */
/** @} */

/**
 * @name Gyroscope Low-pass Filtering options
 *
 * Values for the `gyro-lpf-bw` devicetree property.
 * @{
 */
#define ICM566XX_DT_GYRO_LPF_BW_OFF 0  /**< Filter disabled */
#define ICM566XX_DT_GYRO_LPF_BW_1_4	1  /**< 1/4 ODR */
#define ICM566XX_DT_GYRO_LPF_BW_1_8	2  /**< 1/8 ODR */
#define ICM566XX_DT_GYRO_LPF_BW_1_16	3  /**< 1/16 ODR */
#define ICM566XX_DT_GYRO_LPF_BW_1_32	4  /**< 1/32 ODR */
#define ICM566XX_DT_GYRO_LPF_BW_1_64	5  /**< 1/64 ODR */
#define ICM566XX_DT_GYRO_LPF_BW_1_128	6  /**< 1/128 ODR */
/** @} */

/**
 * @name Accelerometer Low-pass Filtering options
 *
 * Values for the `accel-lpf-bw` devicetree property.
 * @{
 */
#define ICM566XX_DT_ACCEL_LPF_BW_OFF	0  /**< Filter disabled */
#define ICM566XX_DT_ACCEL_LPF_BW_1_4	1  /**< 1/4 ODR */
#define ICM566XX_DT_ACCEL_LPF_BW_1_8	2  /**< 1/8 ODR */
#define ICM566XX_DT_ACCEL_LPF_BW_1_16	3  /**< 1/16 ODR */
#define ICM566XX_DT_ACCEL_LPF_BW_1_32	4  /**< 1/32 ODR */
#define ICM566XX_DT_ACCEL_LPF_BW_1_64	5  /**< 1/64 ODR */
#define ICM566XX_DT_ACCEL_LPF_BW_1_128	6  /**< 1/128 ODR */
/** @} */


/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_TDK_ICM566XX_H_ */
