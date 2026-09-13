/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the TDK InvenSense ICM-20689 IMU.
 * @ingroup icm20689_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ICM20689_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ICM20689_H_

/**
 * @defgroup icm20689_interface ICM20689
 * @ingroup sensor_interface_ext_tdk
 * @brief TDK InvenSense ICM-20689 6-axis IMU
 * @{
 */

/**
 * @name Accelerometer full-scale ranges
 *
 * Values for the `accel-fs` devicetree property.
 * @{
 */
#define ICM20689_DT_ACCEL_FS_2G  2  /**< ±2 g */
#define ICM20689_DT_ACCEL_FS_4G  4  /**< ±4 g */
#define ICM20689_DT_ACCEL_FS_8G  8  /**< ±8 g */
#define ICM20689_DT_ACCEL_FS_16G 16 /**< ±16 g */
/** @} */

/**
 * @name Gyroscope full-scale ranges
 *
 * Values for the `gyro-fs` devicetree property.
 * @{
 */
#define ICM20689_DT_GYRO_FS_250DPS  250  /**< ±250 degrees per second */
#define ICM20689_DT_GYRO_FS_500DPS  500  /**< ±500 degrees per second */
#define ICM20689_DT_GYRO_FS_1000DPS 1000 /**< ±1000 degrees per second */
#define ICM20689_DT_GYRO_FS_2000DPS 2000 /**< ±2000 degrees per second */
/** @} */

/**
 * @name Accelerometer filter configurations
 *
 * Values for the `accel-dlpf` devicetree property.
 *
 * Bits [2:0] represent ACCEL_CONFIG2.A_DLPF_CFG. Bit 3 represents
 * ACCEL_CONFIG2.ACCEL_FCHOICE_B.
 * @{
 */
#define ICM20689_DT_ACCEL_DLPF_218_1HZ_CFG0 0x00 /**< 218.1 Hz, CFG 0 */
#define ICM20689_DT_ACCEL_DLPF_218_1HZ_CFG1 0x01 /**< 218.1 Hz, CFG 1 */
#define ICM20689_DT_ACCEL_DLPF_99HZ         0x02 /**< 99 Hz */
#define ICM20689_DT_ACCEL_DLPF_44_8HZ       0x03 /**< 44.8 Hz */
#define ICM20689_DT_ACCEL_DLPF_21_2HZ       0x04 /**< 21.2 Hz */
#define ICM20689_DT_ACCEL_DLPF_10_2HZ       0x05 /**< 10.2 Hz */
#define ICM20689_DT_ACCEL_DLPF_5_1HZ        0x06 /**< 5.1 Hz */
#define ICM20689_DT_ACCEL_DLPF_420HZ        0x07 /**< 420 Hz */
#define ICM20689_DT_ACCEL_BYPASS_1046HZ     0x08 /**< 1046 Hz, DLPF bypass */
/** @} */

/**
 * @name Gyroscope filter configurations
 *
 * Values for the `gyro-dlpf` devicetree property.
 *
 * This is a packed driver configuration value. Bits [2:0] represent
 * CONFIG.DLPF_CFG and bits [4:3] represent GYRO_CONFIG.FCHOICE_B.
 * @{
 */
#define ICM20689_DT_GYRO_DLPF_250HZ          0x00 /**< 250 Hz, 8 kHz rate */
#define ICM20689_DT_GYRO_DLPF_176HZ          0x01 /**< 176 Hz, 1 kHz rate */
#define ICM20689_DT_GYRO_DLPF_92HZ           0x02 /**< 92 Hz, 1 kHz rate */
#define ICM20689_DT_GYRO_DLPF_41HZ           0x03 /**< 41 Hz, 1 kHz rate */
#define ICM20689_DT_GYRO_DLPF_20HZ           0x04 /**< 20 Hz, 1 kHz rate */
#define ICM20689_DT_GYRO_DLPF_10HZ           0x05 /**< 10 Hz, 1 kHz rate */
#define ICM20689_DT_GYRO_DLPF_5HZ            0x06 /**< 5 Hz, 1 kHz rate */
#define ICM20689_DT_GYRO_DLPF_3281HZ_8KHZ    0x07 /**< 3281 Hz, 8 kHz rate */
#define ICM20689_DT_GYRO_BYPASS_8173HZ_32KHZ 0x08 /**< 8173 Hz, 32 kHz rate */
#define ICM20689_DT_GYRO_BYPASS_3281HZ_32KHZ 0x10 /**< 3281 Hz, 32 kHz rate */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ICM20689_H_ */
