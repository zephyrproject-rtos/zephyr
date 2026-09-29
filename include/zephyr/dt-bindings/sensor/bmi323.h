/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the Bosch BMI323 IMU.
 * @ingroup bmi323_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_BMI323_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_BMI323_H_

/**
 * @brief Bosch BMI323 6-axis IMU
 * @defgroup bmi323_interface BMI323
 * @ingroup sensor_interface_ext_bosch
 * @{
 */

/**
 * @name Sensor power modes
 *
 * Values for the `accel-pwr-mode` and `gyro-pwr-mode` devicetree properties.
 * The accelerometer and the gyroscope encode this field identically.
 * @{
 */
#define BMI323_DT_PWR_MODE_DISABLED  0x00 /**< Powered down */
#define BMI323_DT_PWR_MODE_LOW_POWER 0x04 /**< Duty-cycled low power mode */
#define BMI323_DT_PWR_MODE_HIGH_PERF 0x07 /**< Continuous high performance mode */
/** @} */

/**
 * @name Output data rate options
 *
 * Values for the `accel-odr` and `gyro-odr` devicetree properties.
 * The accelerometer and the gyroscope encode this field identically.
 * @{
 */
#define BMI323_DT_ODR_0_78125_HZ 0x01 /**< 0.78125 Hz */
#define BMI323_DT_ODR_1_5625_HZ  0x02 /**< 1.5625 Hz */
#define BMI323_DT_ODR_3_125_HZ   0x03 /**< 3.125 Hz */
#define BMI323_DT_ODR_6_25_HZ    0x04 /**< 6.25 Hz */
#define BMI323_DT_ODR_12_5_HZ    0x05 /**< 12.5 Hz */
#define BMI323_DT_ODR_25_HZ      0x06 /**< 25 Hz */
#define BMI323_DT_ODR_50_HZ      0x07 /**< 50 Hz */
#define BMI323_DT_ODR_100_HZ     0x08 /**< 100 Hz */
#define BMI323_DT_ODR_200_HZ     0x09 /**< 200 Hz */
#define BMI323_DT_ODR_400_HZ     0x0A /**< 400 Hz */
#define BMI323_DT_ODR_800_HZ     0x0B /**< 800 Hz */
#define BMI323_DT_ODR_1600_HZ    0x0C /**< 1600 Hz */
#define BMI323_DT_ODR_3200_HZ    0x0D /**< 3200 Hz */
#define BMI323_DT_ODR_6400_HZ    0x0E /**< 6400 Hz */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_BMI323_H_ */
