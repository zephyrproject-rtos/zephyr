/*
 * Copyright (c) 2022-2024 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SENSING_SENSOR_TYPES_H_
#define ZEPHYR_INCLUDE_SENSING_SENSOR_TYPES_H_

/**
 * @brief Sensor Types Definition
 *
 * Sensor types definition followed HID standard.
 * https://usb.org/sites/default/files/hutrr39b_0.pdf
 *
 * TODO: will add more types
 *
 * @addtogroup sensing_sensor_types
 * @{
 */

/**
 * sensor category light
 */
#define SENSING_SENSOR_TYPE_LIGHT_AMBIENTLIGHT			0x41

/**
 * @brief Sensor types for motion sensors.
 *
 * These macros define the sensor types for various motion sensors, including 3D accelerometers, 3D
 * gyrometers, and motion detectors.
 */
#define SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D		0x73
#define SENSING_SENSOR_TYPE_MOTION_GYROMETER_3D			0x76
#define SENSING_SENSOR_TYPE_MOTION_MOTION_DETECTOR		0x77

/**
 * @brief Sensor type for custom sensors.
 *
 * This macro defines the sensor type for custom sensors.
 */
#define SENSING_SENSOR_TYPE_OTHER_CUSTOM			0xE1

/**
 * @brief Sensor types for uncalibrated 3D accelerometers and hinge angle sensors.
 *
 * These macros define the sensor types for uncalibrated 3D accelerometers and hinge angle sensors.
 */
#define SENSING_SENSOR_TYPE_MOTION_UNCALIB_ACCELEROMETER_3D	0x240
#define SENSING_SENSOR_TYPE_MOTION_HINGE_ANGLE			0x20B

/**
 * @brief Sensor type for all sensors.
 *
 * This macro defines the sensor type for all sensors.
 */
#define SENSING_SENSOR_TYPE_ALL					0xFFFF

/**
 * @}
 */

#endif /*ZEPHYR_INCLUDE_SENSING_SENSOR_TYPES_H_*/
