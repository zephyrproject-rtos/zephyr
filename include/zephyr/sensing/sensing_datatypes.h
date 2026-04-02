/*
 * Copyright (c) 2022-2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SENSING_DATATYPES_H_
#define ZEPHYR_INCLUDE_SENSING_DATATYPES_H_

#include <stdint.h>
#include <zephyr/dsp/types.h>

/**
 * @defgroup sensing_datatypes Data Types
 * @ingroup sensing_api
 * @brief Sensor data structures used by the sensing subsystem
 *
 * @{
 */

/**
 * @brief Common header for all sensor value payloads.
 *
 * Each sensor value data structure should have this header
 *
 * Here use 'base_timestamp' (uint64_t) and 'timestamp_delta' (uint32_t) to
 * save memory usage in batching mode.
 *
 * The 'base_timestamp' is for readings[0], the 'timestamp_delta' is relation
 * to the previous 'readings'. So,
 * timestamp of readings[0] is
 *	header.base_timestamp + readings[0].timestamp_delta.
 * timestamp of readings[1] is
 *	timestamp of readings[0] + readings[1].timestamp_delta.
 *
 * Since timestamp unit is micro seconds, the max 'timestamp_delta' (uint32_t)
 * is 4295 seconds.
 *
 * If a sensor has batched data where two consecutive readings differ by
 * more than 4295 seconds, the sensor subsystem core will split them
 * across multiple instances of the readings structure, and send multiple
 * events.
 *
 * This concept is borrowed from CHRE:
 * https://cs.android.com/android/platform/superproject/+/master:\
 * system/chre/chre_api/include/chre_api/chre/sensor_types.h
 */
struct sensing_sensor_value_header {
	/** Base timestamp of this data readings, unit is micro seconds */
	uint64_t base_timestamp;
	/** Count of this data readings */
	uint16_t reading_count;
};

/**
 * @brief Sensor value data structure types based on common data types.
 * Suitable for common sensors, such as IMU, Light sensors and orientation sensors.
 */

/**
 * @brief Sensor value data structure for 3-axis sensors.
 * struct sensing_sensor_value_3d_q31 can be used by 3D IMU sensors like:
 * SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D,
 * SENSING_SENSOR_TYPE_MOTION_UNCALIB_ACCELEROMETER_3D,
 * SENSING_SENSOR_TYPE_MOTION_GYROMETER_3D,
 * q31 version
 */
struct sensing_sensor_value_3d_q31 {
	/** Header of the sensor value data structure. */
	struct sensing_sensor_value_header header;
	int8_t shift; /**< The shift value for the q31_t v[3] reading. */
	struct {
		/** Timestamp delta of the reading. Unit is micro seconds. */
		uint32_t timestamp_delta;
		union {
			/**
			 * 3D vector of the reading represented as an array.
			 * For SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D and
			 * SENSING_SENSOR_TYPE_MOTION_UNCALIB_ACCELEROMETER_3D,
			 * the unit is Gs (gravitational force).
			 * For SENSING_SENSOR_TYPE_MOTION_GYROMETER_3D, the unit is degrees.
			 */
			q31_t v[3];
			struct {
				q31_t x; /**< X value of the 3D vector. */
				q31_t y; /**< Y value of the 3D vector. */
				q31_t z; /**< Z value of the 3D vector. */
			};
		};
	} readings[1]; /**< Array of readings. */
};

/**
 * @brief Sensor value data structure for single 1-axis value.
 * struct sensing_sensor_value_uint32 can be used by SENSING_SENSOR_TYPE_LIGHT_AMBIENTLIGHT sensor
 * uint32_t version
 */
struct sensing_sensor_value_uint32 {
	/** Header of the sensor value data structure. */
	struct sensing_sensor_value_header header;
	struct {
		/** Timestamp delta of the reading. Unit is micro seconds. */
		uint32_t timestamp_delta;
		/**
		 * Value of the reading.
		 * For SENSING_SENSOR_TYPE_LIGHT_AMBIENTLIGHT, the unit is luxs.
		 */
		uint32_t v;
	} readings[1];      /**< Array of readings. */
};

/**
 * @brief Sensor value data structure for single 1-axis value.
 * struct sensing_sensor_value_q31 can be used by SENSING_SENSOR_TYPE_MOTION_HINGE_ANGLE sensor
 * q31 version
 */
struct sensing_sensor_value_q31 {
	/** Header of the sensor value data structure. */
	struct sensing_sensor_value_header header;
	int8_t shift; /**< The shift value for the q31_t v reading. */
	struct {
		/** Timestamp delta of the reading. Unit is micro seconds. */
		uint32_t timestamp_delta;
		/**
		 * Value of the reading.
		 * For SENSING_SENSOR_TYPE_MOTION_HINGE_ANGLE, the unit is degrees.
		 */
		q31_t v;
	} readings[1];   /**< Array of readings. */
};

/**
 * @}
 */

#endif /*ZEPHYR_INCLUDE_SENSING_DATATYPES_H_*/
