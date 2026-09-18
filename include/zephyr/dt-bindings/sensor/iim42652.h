/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the TDK InvenSense IIM42652 IMU.
 * @ingroup iim42652_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_IIM42652_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_IIM42652_H_

#include "icm42688.h"

/**
 * @defgroup iim42652_interface IIM42652
 * @ingroup sensor_interface_ext_tdk
 * @brief TDK InvenSense IIM42652 6-axis IMU
 * @{
 */

/**
 * @name Accelerometer power modes
 *
 * Values for the `accel-pwr-mode` devicetree property.
 * @{
 */
#define IIM42652_DT_ACCEL_OFF		ICM42688_DT_ACCEL_OFF /**< Powered down */
#define IIM42652_DT_ACCEL_LP		ICM42688_DT_ACCEL_LP  /**< Low-power mode */
#define IIM42652_DT_ACCEL_LN		ICM42688_DT_ACCEL_LN  /**< Low-noise mode */
/** @} */

/**
 * @name Gyroscope power modes
 *
 * Values for the `gyro-pwr-mode` devicetree property.
 * @{
 */
#define IIM42652_DT_GYRO_OFF		ICM42688_DT_GYRO_OFF     /**< Powered down */
#define IIM42652_DT_GYRO_STANDBY	ICM42688_DT_GYRO_STANDBY /**< Standby */
#define IIM42652_DT_GYRO_LN		ICM42688_DT_GYRO_LN      /**< Low-noise mode */
/** @} */

/**
 * @name Accelerometer full-scale range options
 *
 * Values for the `accel-fs` devicetree property.
 * @{
 */
#define IIM42652_DT_ACCEL_FS_16		ICM42688_DT_ACCEL_FS_16 /**< ±16 g */
#define IIM42652_DT_ACCEL_FS_8		ICM42688_DT_ACCEL_FS_8  /**< ±8 g */
#define IIM42652_DT_ACCEL_FS_4		ICM42688_DT_ACCEL_FS_4  /**< ±4 g */
#define IIM42652_DT_ACCEL_FS_2		ICM42688_DT_ACCEL_FS_2  /**< ±2 g */
/** @} */

/**
 * @name Gyroscope full-scale range options
 *
 * Values for the `gyro-fs` devicetree property.
 * @{
 */
#define IIM42652_DT_GYRO_FS_2000	ICM42688_DT_GYRO_FS_2000   /**< ±2000 dps */
#define IIM42652_DT_GYRO_FS_1000	ICM42688_DT_GYRO_FS_1000   /**< ±1000 dps */
#define IIM42652_DT_GYRO_FS_500		ICM42688_DT_GYRO_FS_500    /**< ±500 dps */
#define IIM42652_DT_GYRO_FS_250		ICM42688_DT_GYRO_FS_250    /**< ±250 dps */
#define IIM42652_DT_GYRO_FS_125		ICM42688_DT_GYRO_FS_125    /**< ±125 dps */
#define IIM42652_DT_GYRO_FS_62_5	ICM42688_DT_GYRO_FS_62_5   /**< ±62.5 dps */
#define IIM42652_DT_GYRO_FS_31_25	ICM42688_DT_GYRO_FS_31_25  /**< ±31.25 dps */
#define IIM42652_DT_GYRO_FS_15_625	ICM42688_DT_GYRO_FS_15_625 /**< ±15.625 dps */
/** @} */

/**
 * @name Accelerometer data rate options
 *
 * Values for the `accel-odr` devicetree property.
 * @{
 */
#define IIM42652_DT_ACCEL_ODR_32000	ICM42688_DT_ACCEL_ODR_32000  /**< 32 kHz */
#define IIM42652_DT_ACCEL_ODR_16000	ICM42688_DT_ACCEL_ODR_16000  /**< 16 kHz */
#define IIM42652_DT_ACCEL_ODR_8000	ICM42688_DT_ACCEL_ODR_8000   /**< 8 kHz */
#define IIM42652_DT_ACCEL_ODR_4000	ICM42688_DT_ACCEL_ODR_4000   /**< 4 kHz */
#define IIM42652_DT_ACCEL_ODR_2000	ICM42688_DT_ACCEL_ODR_2000   /**< 2 kHz */
#define IIM42652_DT_ACCEL_ODR_1000	ICM42688_DT_ACCEL_ODR_1000   /**< 1 kHz */
#define IIM42652_DT_ACCEL_ODR_200	ICM42688_DT_ACCEL_ODR_200    /**< 200 Hz */
#define IIM42652_DT_ACCEL_ODR_100	ICM42688_DT_ACCEL_ODR_100    /**< 100 Hz */
#define IIM42652_DT_ACCEL_ODR_50	ICM42688_DT_ACCEL_ODR_50     /**< 50 Hz */
#define IIM42652_DT_ACCEL_ODR_25	ICM42688_DT_ACCEL_ODR_25     /**< 25 Hz */
#define IIM42652_DT_ACCEL_ODR_12_5	ICM42688_DT_ACCEL_ODR_12_5   /**< 12.5 Hz */
#define IIM42652_DT_ACCEL_ODR_6_25	ICM42688_DT_ACCEL_ODR_6_25   /**< 6.25 Hz */
#define IIM42652_DT_ACCEL_ODR_3_125	ICM42688_DT_ACCEL_ODR_3_125  /**< 3.125 Hz */
#define IIM42652_DT_ACCEL_ODR_1_5625	ICM42688_DT_ACCEL_ODR_1_5625 /**< 1.5625 Hz */
#define IIM42652_DT_ACCEL_ODR_500	ICM42688_DT_ACCEL_ODR_500    /**< 500 Hz */
/** @} */

/**
 * @name Gyroscope data rate options
 *
 * Values for the `gyro-odr` devicetree property.
 * @{
 */
#define IIM42652_DT_GYRO_ODR_32000	ICM42688_DT_GYRO_ODR_32000 /**< 32 kHz */
#define IIM42652_DT_GYRO_ODR_16000	ICM42688_DT_GYRO_ODR_16000 /**< 16 kHz */
#define IIM42652_DT_GYRO_ODR_8000	ICM42688_DT_GYRO_ODR_8000  /**< 8 kHz */
#define IIM42652_DT_GYRO_ODR_4000	ICM42688_DT_GYRO_ODR_4000  /**< 4 kHz */
#define IIM42652_DT_GYRO_ODR_2000	ICM42688_DT_GYRO_ODR_2000  /**< 2 kHz */
#define IIM42652_DT_GYRO_ODR_1000	ICM42688_DT_GYRO_ODR_1000  /**< 1 kHz */
#define IIM42652_DT_GYRO_ODR_200	ICM42688_DT_GYRO_ODR_200   /**< 200 Hz */
#define IIM42652_DT_GYRO_ODR_100	ICM42688_DT_GYRO_ODR_100   /**< 100 Hz */
#define IIM42652_DT_GYRO_ODR_50		ICM42688_DT_GYRO_ODR_50    /**< 50 Hz */
#define IIM42652_DT_GYRO_ODR_25		ICM42688_DT_GYRO_ODR_25    /**< 25 Hz */
#define IIM42652_DT_GYRO_ODR_12_5	ICM42688_DT_GYRO_ODR_12_5  /**< 12.5 Hz */
#define IIM42652_DT_GYRO_ODR_500	ICM42688_DT_GYRO_ODR_500   /**< 500 Hz */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_IIM42652_H_ */
