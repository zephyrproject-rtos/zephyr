/**
 * @file drivers/sensor_types.h
 *
 * @brief Public APIs for the sensor types.
 *
 * The SENSOR_TYPE_* defines are the sensor types supported. Additional types can be added at
 * SENSOR_TYPE_VENDOR_START.
 */

/*
 * Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INCLUDE_DRIVERS_SENSOR_TYPES_H_
#define INCLUDE_DRIVERS_SENSOR_TYPES_H_

#include <zephyr/math/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Start value for all of the vendor-defined private sensors.
 */
#define SENSOR_TYPE_VENDOR_START UINT8_C(192)

/**
 * Accelerometer sensor returning data in m/s^2.
 */
#define SENSOR_TYPE_ACCELEROMETER UINT8_C(1)

/**
 * Instantaneous motion detection.
 *
 * This is a one-shot sensor.
 */
#define SENSOR_TYPE_INSTANT_MOTION_DETECT UINT8_C(2)

/**
 * Stationary detection.
 *
 * This is a one-shot sensor.
 */
#define SENSOR_TYPE_STATIONARY_DETECT UINT8_C(3)

/**
 * Gyroscope sensor returning data in radians / sec
 */
#define SENSOR_TYPE_GYROSCOPE UINT8_C(6)

/**
 * Magnetometer sensor returning data in Teslas.
 */
#define SENSOR_TYPE_GEOMAGNETIC_FIELD UINT8_C(8)

/**
 * Barometric pressure sensor returning data in Pascals.
 */
#define SENSOR_TYPE_PRESSURE UINT8_C(10)

/**
 * Ambient light sensor returning data in lux.
 *
 * This is an on-change sensor.
 */
#define SENSOR_TYPE_LIGHT UINT8_C(12)

/**
 * Proximity detection.
 *
 * This is an on-change sensor.
 */
#define SENSOR_TYPE_PROXIMITY UINT8_C(13)

/**
 * Step detection.
 */
#define SENSOR_TYPE_STEP_DETECT UINT8_C(23)

/**
 * Step counter.
 *
 * This is an on-change sensor.
 */
#define SENSOR_TYPE_STEP_COUNTER UINT8_C(24)

/**
 * Accelerometer temperature sensor returning the accelerometer's temperature in Kelvin.
 */
#define SENSOR_TYPE_ACCELEROMETER_TEMPERATURE UINT8_C(56)

/**
 * Gyroscope temperature sensor returning the gyroscope's temperature in Kelvin.
 */
#define SENSOR_TYPE_GYROSCOPE_TEMPERATURE UINT8_C(57)

/**
 * Magnetometer temperature sensor returning the magnetometer's temperature in Kelvin.
 */
#define SENSOR_TYPE_GEOMAGNETIC_FIELD_TEMPERATURE UINT8_C(58)

BUILD_ASSERT(SENSOR_TYPE_GEOMAGNETIC_FIELD_TEMPERATURE < SENSOR_TYPE_VENDOR_START,
	     "Too many sensor types");

struct sensor_data_header {
	/** The number of allocated `readings` elements. */
	const uint16_t reading_size;

	/** The base timestamp, in nanoseconds. */
	uint64_t base_timestamp;

	/** The number of valid elements in the 'readings' array. */
	uint16_t reading_count;
};

/**
 * @brief A set of 3D data points from the sensor.
 *
 * This is used by:
 * - SENSOR_TYPE_ACCELEROMETER
 * - SENSOR_TYPE_GYROSCOPE
 * - SENSOR_TYPE_MAGNETOMETER
 * - SENSOR_TYPE_GEOMAGNETIC_FIELD
 */
struct sensor_three_axis_data {
	/** Header data */
	struct sensor_data_header header;

	/** The data from the sensor */
	struct sensor_three_axis_sample_data {
		union {
			fp_t values[3];
			struct {
				fp_t x;
				fp_t y;
				fp_t z;
			};
		};
	} readings[1];
};

/**
 * @brief A raw single byte data points from the sensor.
 *
 * This is used by:
 * - SENSOR_TYPE_PROXIMITY
 */
struct sensor_byte_data {
	/** Header data */
	struct sensor_data_header header;

	/** The data from the sensor */
	struct sensor_byte_sample_data {
		union {
			uint8_t value;
			struct {
				uint8_t is_near : 1;
				uint8_t padding : 7;
			};
		};
	} readings[1];
};

/**
 * This is used by:
 * - SENSOR_TYPE_LIGHT
 * - SENSOR_TYPE_PRESSURE
 * - SENSOR_TYPE_ACCELEROMETER_TEMPERATURE
 * - SENSOR_TYPE_GYROSCOPE_TEMPERATURE
 * - SENSOR_TYPE_MAGNETIC_FIELD_TEMPERATURE
 */
struct sensor_float_data {
	struct sensor_data_header header;
	struct sensor_float_sample_data {
		fp_t value;
	} readings[1];
};

/**
 * This is used by:
 * - SENSOR_TYPE_STEP_COUNTER
 */
struct sensor_uint64_data {
	struct sensor_data_header header;
	struct sensor_uint64_sample_data {
		uint64_t value;
	} readings[1];
};

struct sensor_raw_data {
	/** Header data */
	struct sensor_data_header header;

	/** The raw data from the sensor */
	uint8_t readings[1];
};

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define Z_READING_SIZE_VALUE(n) (n & 0xff), ((n >> 8) & 0xff)
#else
#define Z_READING_SIZE_VALUE(n) ((n >> 8) & 0xff), (n & 0xff)
#endif

/**
 * @brief Utility macro for defining a new struct sensor_data with more than 1 reading
 *
 * To support variable size readings, the sensor_data's reading_size needs to be set in the header.
 * This field is immutable to avoid errors, so some creative memory allocation is needed. Use this
 * macro like:
 *
 * @code
 * SENSOR_DATA(struct sensor_three_axis_data, my_data, 5); //< Allocate a struct sensor_data with
 *                                                         //< room for 5 readings.
 * @endcode
 *
 * @param type The type of sensor data struct to allocate
 * @param name The name of the variable to create
 * @param readings_size The number of readings to allocate
 */
#define SENSOR_DATA(type, name, readings_size)                                                     \
	uint8_t __##name##_buffer[sizeof(type) +                                                   \
				  (readings_size - 1) *                                            \
					  (sizeof(type) - sizeof(struct sensor_data_header))] = {  \
		Z_READING_SIZE_VALUE(readings_size)                                                \
	};                                                                                         \
	type *name = (type *)__##name##_buffer

/**
 * @brief Utility macro for defining a new struct sensor_raw_data with more than 1 byte
 *
 * To support more than 1 byte of data reading, the sensor_raw_data's reading_size needs to be set
 * in the header. The field is immutable to avoid errors, so some creative memory allocation is
 * needed. Use this macro like:
 *
 * @code
 * SENSOR_RAW_DATA(my_data, 2000); //< Allocate a struct sensor_raw_data with room for 2000 bytes
 * @endcode
 *
 * @param name The name of the variable to create
 * @param buffer_size The number of bytes to allocate
 */
#define SENSOR_RAW_DATA(name, buffer_size)                                                         \
	uint8_t __##name##_buffer[sizeof(struct sensor_raw_data) + buffer_size - 1] = {            \
		Z_READING_SIZE_VALUE(buffer_size)                                                  \
	};                                                                                         \
	struct sensor_raw_data *name = (struct sensor_raw_data *)__##name##_buffer

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_DRIVERS_SENSOR_TYPES_H_ */
