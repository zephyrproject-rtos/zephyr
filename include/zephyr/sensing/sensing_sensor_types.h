/*
 * Copyright (c) 2022-2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SENSING_SENSOR_TYPES_H_
#define ZEPHYR_INCLUDE_SENSING_SENSOR_TYPES_H_

/**
 * @defgroup sensing_sensor_types Sensor Types (Sensing)
 * @ingroup sensing_api
 *
 * @brief Sensor type identifiers used by the sensing subsystem.
 *
 * Sensor types definition followed HID standard.
 * https://usb.org/sites/default/files/hutrr39b_0.pdf
 *
 * TODO: will add more types
 * @{
 */

/**
 * @name Light sensors
 * @{
 */

/** Sensor type for ambient light sensors. */
#define SENSING_SENSOR_TYPE_LIGHT_AMBIENTLIGHT			0x41

/** @} */

/**
 * @name Motion sensors
 * @{
 */

/** Sensor type for 3D accelerometers. */
#define SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D		0x73
/** Sensor type for 3D gyrometers. */
#define SENSING_SENSOR_TYPE_MOTION_GYROMETER_3D			0x76
/** Sensor type for motion detectors. */
#define SENSING_SENSOR_TYPE_MOTION_MOTION_DETECTOR		0x77
/** Sensor type for uncalibrated 3D accelerometers. */
#define SENSING_SENSOR_TYPE_MOTION_UNCALIB_ACCELEROMETER_3D     0x240
/** Sensor type for hinge angle sensors. */
#define SENSING_SENSOR_TYPE_MOTION_HINGE_ANGLE                  0x20B

/** @} */

/**
 * @name Other sensors
 * @{
 */

/** Sensor type for custom sensors. */
#define SENSING_SENSOR_TYPE_OTHER_CUSTOM 0xE1

/** @} */

/**
 * @brief Sensor type for all sensors.
 *
 * This macro defines the sensor type for all sensors.
 *
 * @note This value is not a valid sensor type and is used as a sentinel value to indicate all
 *       sensor types.
 */
#define SENSING_SENSOR_TYPE_ALL					0xFFFF

/**
 * @}
 */

#endif /*ZEPHYR_INCLUDE_SENSING_SENSOR_TYPES_H_*/
