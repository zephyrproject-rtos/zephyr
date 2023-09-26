/*
 * Copyright (c) 2022-2023 Intel Corporation.
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
 * sensor category motion
 */
#define SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D		0x73
#define SENSING_SENSOR_TYPE_MOTION_GYROMETER_3D			0x76
#define SENSING_SENSOR_TYPE_MOTION_MOTION_DETECTOR		0x77


/**
 * sensor category other
 */
#define SENSING_SENSOR_TYPE_OTHER_CUSTOM			0xE1

#define SENSING_SENSOR_TYPE_MOTION_UNCALIB_ACCELEROMETER_3D	0x240

#define SENSING_SENSOR_TYPE_MOTION_HINGE_ANGLE			0x20B

#define SENSING_SENSOR_TYPE_ALL					0xFFFF

/**
 * @}
 */

#endif /*ZEPHYR_INCLUDE_SENSING_SENSOR_TYPES_H_*/
