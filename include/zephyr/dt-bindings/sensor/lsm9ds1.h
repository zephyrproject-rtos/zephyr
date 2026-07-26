/*
 * Copyright (c) 2024 Bootlin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the ST LSM9DS1 IMU and magnetometer.
 * @ingroup lsm9ds1_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_LSM9DS1_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_LSM9DS1_H_

/**
 * @defgroup lsm9ds1_interface LSM9DS1
 * @ingroup sensor_interface_ext_st
 * @brief STMicroelectronics LSM9DS1 9-axis IMU (accel, gyro, magnetometer)
 * @{
 */

/**
 * @name Accelerometer full-scale range
 *
 * Values for the `accel-range` devicetree property.
 * @{
 */
#define LSM9DS1_DT_FS_2G  0 /**< ±2 g */
#define LSM9DS1_DT_FS_16G 1 /**< ±16 g */
#define LSM9DS1_DT_FS_4G  2 /**< ±4 g */
#define LSM9DS1_DT_FS_8G  3 /**< ±8 g */
/** @} */

/**
 * @name Gyroscope full-scale range
 *
 * Values for the `gyro-range` devicetree property.
 * @{
 */
#define LSM9DS1_DT_FS_245DPS  0 /**< ±245 dps */
#define LSM9DS1_DT_FS_500DPS  1 /**< ±500 dps */
#define LSM9DS1_DT_FS_2000DPS 3 /**< ±2000 dps */
/** @} */

/**
 * @name Accelerometer and gyroscope output data rate
 *
 * Values for the `imu-odr` devicetree property.
 *
 * Each value selects an accelerometer (XL) and gyroscope (GY) rate
 * combination. The _LP suffix selects the gyroscope low-power mode.
 * @{
 */
#define LSM9DS1_IMU_OFF            0x00 /**< Accelerometer and gyroscope off */
#define LSM9DS1_GY_OFF_XL_10Hz     0x10 /**< GY off, XL 10 Hz */
#define LSM9DS1_GY_OFF_XL_50Hz     0x20 /**< GY off, XL 50 Hz */
#define LSM9DS1_GY_OFF_XL_119Hz    0x30 /**< GY off, XL 119 Hz */
#define LSM9DS1_GY_OFF_XL_238Hz    0x40 /**< GY off, XL 238 Hz */
#define LSM9DS1_GY_OFF_XL_476Hz    0x50 /**< GY off, XL 476 Hz */
#define LSM9DS1_GY_OFF_XL_952Hz    0x60 /**< GY off, XL 952 Hz */
#define LSM9DS1_XL_OFF_GY_14Hz9    0x01 /**< XL off, GY 14.9 Hz */
#define LSM9DS1_XL_OFF_GY_59Hz5    0x02 /**< XL off, GY 59.5 Hz */
#define LSM9DS1_XL_OFF_GY_119Hz    0x03 /**< XL off, GY 119 Hz */
#define LSM9DS1_XL_OFF_GY_238Hz    0x04 /**< XL off, GY 238 Hz */
#define LSM9DS1_XL_OFF_GY_476Hz    0x05 /**< XL off, GY 476 Hz */
#define LSM9DS1_XL_OFF_GY_952Hz    0x06 /**< XL off, GY 952 Hz */
#define LSM9DS1_IMU_14Hz9          0x11 /**< XL and GY 14.9 Hz */
#define LSM9DS1_IMU_59Hz5          0x22 /**< XL and GY 59.5 Hz */
#define LSM9DS1_IMU_119Hz          0x33 /**< XL and GY 119 Hz */
#define LSM9DS1_IMU_238Hz          0x44 /**< XL and GY 238 Hz */
#define LSM9DS1_IMU_476Hz          0x55 /**< XL and GY 476 Hz */
#define LSM9DS1_IMU_952Hz          0x66 /**< XL and GY 952 Hz */
#define LSM9DS1_XL_OFF_GY_14Hz9_LP 0x81 /**< XL off, GY 14.9 Hz (low-power) */
#define LSM9DS1_XL_OFF_GY_59Hz5_LP 0x82 /**< XL off, GY 59.5 Hz (low-power) */
#define LSM9DS1_XL_OFF_GY_119Hz_LP 0x83 /**< XL off, GY 119 Hz (low-power) */
#define LSM9DS1_IMU_14Hz9_LP       0x91 /**< XL and GY 14.9 Hz (low-power) */
#define LSM9DS1_IMU_59Hz5_LP       0xA2 /**< XL and GY 59.5 Hz (low-power) */
#define LSM9DS1_IMU_119Hz_LP       0xB3 /**< XL and GY 119 Hz (low-power) */
/** @} */

/**
 * @name Magnetometer full-scale range
 *
 * Values for the `mag-range` devicetree property.
 * @{
 */
#define LSM9DS1_DT_FS_4Ga  0 /**< ±4 gauss */
#define LSM9DS1_DT_FS_8Ga  1 /**< ±8 gauss */
#define LSM9DS1_DT_FS_12Ga 2 /**< ±12 gauss */
#define LSM9DS1_DT_FS_16Ga 3 /**< ±16 gauss */
/** @} */

/**
 * @name Magnetometer output data rate
 *
 * Values for the `mag-odr` devicetree property.
 *
 * LP: low-power, MP: medium-performance, HP: high-performance,
 * UHP: ultra-high-performance mode.
 * @{
 */
#define LSM9DS1_MAG_POWER_DOWN 0xC0 /**< Power-down */
#define LSM9DS1_MAG_LP_0Hz625  0x00 /**< LP, 0.625 Hz */
#define LSM9DS1_MAG_LP_1Hz25   0x01 /**< LP, 1.25 Hz */
#define LSM9DS1_MAG_LP_2Hz5    0x02 /**< LP, 2.5 Hz */
#define LSM9DS1_MAG_LP_5Hz     0x03 /**< LP, 5 Hz */
#define LSM9DS1_MAG_LP_10Hz    0x04 /**< LP, 10 Hz */
#define LSM9DS1_MAG_LP_20Hz    0x05 /**< LP, 20 Hz */
#define LSM9DS1_MAG_LP_40Hz    0x06 /**< LP, 40 Hz */
#define LSM9DS1_MAG_LP_80Hz    0x07 /**< LP, 80 Hz */
#define LSM9DS1_MAG_MP_0Hz625  0x10 /**< MP, 0.625 Hz */
#define LSM9DS1_MAG_MP_1Hz25   0x11 /**< MP, 1.25 Hz */
#define LSM9DS1_MAG_MP_2Hz5    0x12 /**< MP, 2.5 Hz */
#define LSM9DS1_MAG_MP_5Hz     0x13 /**< MP, 5 Hz */
#define LSM9DS1_MAG_MP_10Hz    0x14 /**< MP, 10 Hz */
#define LSM9DS1_MAG_MP_20Hz    0x15 /**< MP, 20 Hz */
#define LSM9DS1_MAG_MP_40Hz    0x16 /**< MP, 40 Hz */
#define LSM9DS1_MAG_MP_80Hz    0x17 /**< MP, 80 Hz */
#define LSM9DS1_MAG_HP_0Hz625  0x20 /**< HP, 0.625 Hz */
#define LSM9DS1_MAG_HP_1Hz25   0x21 /**< HP, 1.25 Hz */
#define LSM9DS1_MAG_HP_2Hz5    0x22 /**< HP, 2.5 Hz */
#define LSM9DS1_MAG_HP_5Hz     0x23 /**< HP, 5 Hz */
#define LSM9DS1_MAG_HP_10Hz    0x24 /**< HP, 10 Hz */
#define LSM9DS1_MAG_HP_20Hz    0x25 /**< HP, 20 Hz */
#define LSM9DS1_MAG_HP_40Hz    0x26 /**< HP, 40 Hz */
#define LSM9DS1_MAG_HP_80Hz    0x27 /**< HP, 80 Hz */
#define LSM9DS1_MAG_UHP_0Hz625 0x30 /**< UHP, 0.625 Hz */
#define LSM9DS1_MAG_UHP_1Hz25  0x31 /**< UHP, 1.25 Hz */
#define LSM9DS1_MAG_UHP_2Hz5   0x32 /**< UHP, 2.5 Hz */
#define LSM9DS1_MAG_UHP_5Hz    0x33 /**< UHP, 5 Hz */
#define LSM9DS1_MAG_UHP_10Hz   0x34 /**< UHP, 10 Hz */
#define LSM9DS1_MAG_UHP_20Hz   0x35 /**< UHP, 20 Hz */
#define LSM9DS1_MAG_UHP_40Hz   0x36 /**< UHP, 40 Hz */
#define LSM9DS1_MAG_UHP_80Hz   0x37 /**< UHP, 80 Hz */
#define LSM9DS1_MAG_UHP_155Hz  0x38 /**< UHP, 155 Hz */
#define LSM9DS1_MAG_HP_300Hz   0x28 /**< HP, 300 Hz */
#define LSM9DS1_MAG_MP_560Hz   0x18 /**< MP, 560 Hz */
#define LSM9DS1_MAG_LP_1000Hz  0x08 /**< LP, 1000 Hz */
#define LSM9DS1_MAG_ONE_SHOT   0x70 /**< Single (one-shot) conversion */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_LSM9DS1_H_ */
