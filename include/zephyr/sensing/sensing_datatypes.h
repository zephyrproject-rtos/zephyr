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
 * @brief Data Types
 * @addtogroup sensing_datatypes
 * @{
 */

/**
 * @struct sensing_sensor_value_header
 * @brief sensor value header
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
	/** base timestamp of this data readings, unit is micro seconds */
	uint64_t base_timestamp;
	/** count of this data readings */
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
	struct sensing_sensor_value_header header;
	int8_t shift;
	struct {
		uint32_t timestamp_delta;
		union {
			q31_t v[3];
			struct {
				q31_t x;
				q31_t y;
				q31_t z;
			};
		};
	} readings[1];
};

/**
 * @brief Sensor value data structure for single 1-axis value.
 * struct sensing_sensor_value_uint32 can be used by SENSING_SENSOR_TYPE_LIGHT_AMBIENTLIGHT sensor
 * uint32_t version
 */
struct sensing_sensor_value_uint32 {
	struct sensing_sensor_value_header header;
	struct {
		uint32_t timestamp_delta;
		uint32_t v;
	} readings[1];
};

/**
 * @brief Sensor value data structure for single 1-axis value.
 * struct sensing_sensor_value_q31 can be used by SENSING_SENSOR_TYPE_MOTION_HINGE_ANGLE sensor
 * q31 version
 */
struct sensing_sensor_value_q31 {
	int8_t shift;
	struct sensing_sensor_value_header header;
	struct {
		uint32_t timestamp_delta;
		q31_t v;
	} readings[1];
};


/**
 * @}
 */

#endif /*ZEPHYR_INCLUDE_SENSING_DATATYPES_H_*/
