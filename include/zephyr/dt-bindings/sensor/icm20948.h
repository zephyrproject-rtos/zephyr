/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree constants for the TDK ICM-20948 sensor
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_TDK_ICM20948_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_TDK_ICM20948_H_

/**
 * @defgroup ICM20948 Invensense (TDK) ICM20948 DT Options
 * @ingroup sensor_interface
 * @{
 */

/**
 * @defgroup ICM20948_ACCEL_CONFIG_FS_SEL_MASK Accelerometer full-scale options
 * @{
 */
#define ICM20948_DT_ACCEL_FS_2  0 /**< +/- 2g */
#define ICM20948_DT_ACCEL_FS_4  1 /**< +/- 4g */
#define ICM20948_DT_ACCEL_FS_8  2 /**< +/- 8g */
#define ICM20948_DT_ACCEL_FS_16 3 /**< +/- 16g */
/** @} */

/**
 * @defgroup ICM20948_GYRO_CONFIG_1_FS_SEL_MASK Gyroscope full-scale options
 * @{
 */
#define ICM20948_DT_GYRO_FS_250  0 /**< +/- 250 dps */
#define ICM20948_DT_GYRO_FS_500  1 /**< +/- 500 dps */
#define ICM20948_DT_GYRO_FS_1000 2 /**< +/- 1000 dps */
#define ICM20948_DT_GYRO_FS_2000 3 /**< +/- 2000 dps */
/** @} */

/**
 * @defgroup ICM20948_GYRO_LPF Gyroscope Low-pass Filtering options
 * @{
 */
#define ICM20948_DT_GYRO_LPF_BW_197HZ         0 /**< 197 Hz bandwidth */
#define ICM20948_DT_GYRO_LPF_BW_152HZ         1 /**< 152 Hz bandwidth */
#define ICM20948_DT_GYRO_LPF_BW_120HZ         2 /**< 120 Hz bandwidth */
#define ICM20948_DT_GYRO_LPF_BW_51HZ          3 /**< 51 Hz bandwidth */
#define ICM20948_DT_GYRO_LPF_BW_24HZ          4 /**< 24 Hz bandwidth */
#define ICM20948_DT_GYRO_LPF_BW_12HZ          5 /**< 12 Hz bandwidth */
#define ICM20948_DT_GYRO_LPF_BW_6HZ           6 /**< 6 Hz bandwidth */
#define ICM20948_DT_GYRO_LPF_BW_361HZ         7 /**< 361 Hz bandwidth */
#define ICM20948_DT_GYRO_LPF_BW_12106HZ_NOLPF 8 /**< 12106 Hz, no low-pass filter */
/** @} */

/**
 * @defgroup ICM20948_ACCEL_CONFIG_DLPFCFG_MASK Accelerometer Low-pass Filtering options
 * @{
 */
#define ICM20948_DT_ACCEL_LPF_BW_246HZ        0 /**< 246 Hz bandwidth */
#define ICM20948_DT_ACCEL_LPF_BW_246HZ_DUP    1 /**< 246 Hz bandwidth (duplicate) */
#define ICM20948_DT_ACCEL_LPF_BW_111HZ        2 /**< 111 Hz bandwidth */
#define ICM20948_DT_ACCEL_LPF_BW_50HZ         3 /**< 50 Hz bandwidth */
#define ICM20948_DT_ACCEL_LPF_BW_24HZ         4 /**< 24 Hz bandwidth */
#define ICM20948_DT_ACCEL_LPF_BW_12HZ         5 /**< 12 Hz bandwidth */
#define ICM20948_DT_ACCEL_LPF_BW_6HZ          6 /**< 6 Hz bandwidth */
#define ICM20948_DT_ACCEL_LPF_BW_473HZ        7 /**< 473 Hz bandwidth */
#define ICM20948_DT_ACCEL_LPF_BW_1209HZ_NOLPF 8 /**< 1209 Hz, no low-pass filter */
/** @} */

/**
 * @defgroup AK09916_CNTL2_MODE_MASK Magnetometer power mode options
 * @{
 */
#define ICM20948_DT_MAG_MODE_DISABLED 0 /**< Magnetometer disabled */
#define ICM20948_DT_MAG_MODE_10HZ     2 /**< 10 Hz continuous measurement */
#define ICM20948_DT_MAG_MODE_20HZ     4 /**< 20 Hz continuous measurement */
#define ICM20948_DT_MAG_MODE_50HZ     6 /**< 50 Hz continuous measurement */
#define ICM20948_DT_MAG_MODE_100HZ    8 /**< 100 Hz continuous measurement */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_TDK_ICM20948_H_ */
