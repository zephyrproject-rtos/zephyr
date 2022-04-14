/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SENSS_SENSOR_TYPES_H_
#define SENSS_SENSOR_TYPES_H_

/**
 * @brief Sensor Types Definition
 *
 * Sensor types definition followed HID standard.
 * https://usb.org/sites/default/files/hutrr39b_0.pdf
 *
 * TODO: will add more types
 *
 * @addtogroup senss_sensor_types
 * @{
 */

/**
 * sensor category light
 */
#define SENSS_SENSOR_TYPE_LIGHT_AMBIENTLIGHT			0x41

/**
 * sensor category motion
 */
#define SENSS_SENSOR_TYPE_MOTION_ACCELEROMETER_3D		0x73
#define SENSS_SENSOR_TYPE_MOTION_MOTION_DETECTOR		0x77


/**
 * sensor category other
 */
#define SENSS_SENSOR_TYPE_OTHER_CUSTOM				0xE1

#define SENSS_SENSOR_TYPE_MOTION_UNCALIB_ACCELEROMETER_3D	0x240

#define SENSS_SENSOR_TYPE_MOTION_HINGE_ANGLE			0x20B

#define SENSS_SENSOR_TYPE_ALL					0xFFFF

/**
 * @}
 */

#endif /* SENSS_SENSOR_TYPES_H_ */
