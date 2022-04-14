/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SENSS_DATATYPES_H_
#define SENSS_DATATYPES_H_

#include <stdint.h>

/**
 * @brief Data Types
 * @addtogroup senss_datatypes
 * @{
 */

/**
 * @struct senss_sensor_value_header
 * @brief sensor value header
 *
 * Each sensor value data structure should have this header
 *
 * Here use 'base_timestamp' (uint64_t) and 'timestampe_delta' (uint32_t) to
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
struct senss_sensor_value_header {
	/** base timestamp of this data readings, unit is micro seconds */
	uint64_t base_timestamp;
	/** count of this data readings */
	uint16_t reading_count;
};

/**
 * @brief Sensor value data structure types based on common data types.
 * Suitable for common sensors, such as IMU, Light sensors and orientation sensors.
 *
 * Sensor subsystem will use scaled fixed point data structure for all sensor values,
 * aligned the HID spec, using the format v*10^x to present the decimal value,
 * where the v is integer number, either int8/uint8, int16/uint6, or int32/uint32, depends on
 * required sensor data precision.
 *
 * The scale unit exponent x is int8 type int8 type with encoding meanings (page 68 of
 * HID spec https://usb.org/sites/default/files/hutrr39b_0.pdf):
 *
 * Unit Exponet argument       Power of Ten (Scientific Notation)
 * 0x00			       1 * 10E0
 * 0x01			       1 * 10E1
 * 0x02			       1 * 10E2
 * 0x03			       1 * 10E3
 * 0x04			       1 * 10E4
 * 0x05			       1 * 10E5
 * 0x06			       1 * 10E6
 * 0x07			       1 * 10E7
 * 0x08			       1 * 10E-8
 * 0x09			       1 * 10E-7
 * 0x0A			       1 * 10E-6
 * 0x0B			       1 * 10E-5
 * 0x0C			       1 * 10E-4
 * 0x0D			       1 * 10E-3
 * 0x0E			       1 * 10E-2
 * 0x0F			       1 * 10E-1
 *
 * So, we can have below data present ranges:
 *
 * int8 type v:    [-128, 127] *10^[-8, 7]
 * uint8 type v:   [0,	255] * 10^[-8, 7]
 * int16 type v:   [-32768, 32767] * 10^[-8, 7]
 * uint16 type v:  [0,	65535\r] * 10^[-8, 7]
 * int32 type v:   [-2147483648,  2147483647] * 10^[-8, 7]
 * uint32 type v:  [0,	4294967295] * 10^[-8, 7]
 * int64 type v:   [-9223372036854775808,  9223372036854775807] * 10^[-8, 7]
 * uint64 type v:  [0,	18446744073709551615] * 10^[-8, 7]
 *
 * To simple the data structure definition and save store memory, only keep v in code definitions,
 * scale exponent x will defined in doc and spec,  but not explicitly present in code, for scenarios
 * which need transfer to decimal value, such as in a algorithm process, need base on the sensor
 * type and according the doc/spec to get the right scale exponent value x.
 *
 * A example in doc and spec can be like:
 * 3D accelerometer:
 *	Data Fields    Type	Unit	    Unit Exponent   Typical Range     Description
 *	data[0]        int32	micro g     -6		    +/-4*10^6	      x axis acceleration
 *	data[1]        int32	micro g     -6		    +/-4*10^6	      y axis acceleration
 *	data[2]        int32	micro g     -6		    +/-4*10^6	      z axis acceleration
 *
 * Ambient Light:
 *	Data Fields    Type	Unit	    Unit Exponent   Typical Range     Description
 *	data[0]        uint32	milli lux   -3		    [0, 10000] *10^3  Ambient light lux level
 *
 * The complete doc/spec should describe all supported sensors like above example.
 */

/**
 * @brief Sensor value data structure for quaternion.
 * int16 version.
 */
struct senss_sensor_value_quaternion_int16 {
	struct senss_sensor_value_header header;
	struct {
		uint32_t timestamp_delta;
		int16_t  x;
		int16_t  y;
		int16_t  z;
		int16_t  w;
	} readings[1];
};

/**
 * @brief Sensor value data structure for 3-axis sensors.
 * int32 version
 */
struct senss_sensor_value_3d_int32 {
	struct senss_sensor_value_header header;
	struct {
		uint32_t timestamp_delta;
		union {
			int32_t v[3];
			struct {
				int32_t x;
				int32_t y;
				int32_t z;
			};
		};
	} readings[1];
};

/**
 * @brief Sensor value data structure for un-calibrated 3-axis sensors.
 * int32 version
 */
struct senss_sensor_value_uncalib_3d_int32 {
	struct senss_sensor_value_header header;
	struct {
		uint32_t timestamp_delta;
		union {
			int32_t v[3];
			struct {
				int32_t x;
				int32_t y;
				int32_t z;
			};
		};
		union {
			int32_t bias[3];
			struct {
				int32_t bias_x;
				int32_t bias_y;
				int32_t bias_z;
			};
		};
	} readings[1];
};

/**
 * @brief Sensor value data structure for 2-axis sensors.
 * int32 version
 */
struct senss_sensor_value_2d_int32 {
	struct senss_sensor_value_header header;
	struct {
		uint32_t timestamp_delta;
		union {
			int32_t v[2];
			struct {
				int32_t x;
				int32_t y;
			};
		};
	} readings[1];
};

/**
 * @brief Sensor value data structure for un-calibrated 2-axis sensors.
 * int32 version
 */
struct senss_sensor_value_uncalib_2d_int32 {
	struct senss_sensor_value_header header;
	struct {
		uint32_t timestamp_delta;
		union {
			int32_t v[2];
			struct {
				int32_t x;
				int32_t y;
			};
		};
		union {
			int32_t bias[2];
			struct {
				int32_t bias_x;
				int32_t bias_y;
			};
		};
	} readings[1];
};

/**
 * @brief Sensor value data structure for single 1-axis value.
 * int64 version
 */
struct senss_sensor_value_int64 {
	struct senss_sensor_value_header header;
	struct {
		uint32_t timestamp_delta;
		int64_t v;
	} readings[1];
};

/**
 * @brief Sensor value data structure for single 1-axis value.
 * uint64 version
 */
struct senss_sensor_value_uint64 {
	struct senss_sensor_value_header header;
	struct {
		uint32_t timestamp_delta;
		uint64_t v;
	} readings[1];
};

/**
 * @brief Sensor value data structure for single 1-axis value.
 * int32 version
 */
struct senss_sensor_value_int32 {
	struct senss_sensor_value_header header;
	struct {
		uint32_t timestamp_delta;
		int32_t v;
	} readings[1];
};

/**
 * @brief Sensor value data structure for single 1-axis value.
 * uint32 version
 */
struct senss_sensor_value_uint32 {
	struct senss_sensor_value_header header;
	struct {
		uint32_t timestamp_delta;
		uint32_t v;
	} readings[1];
};

/**
 * @brief Sensor value data structure for single 1-axis value.
 * int16 version
 */
struct senss_sensor_value_int16 {
	struct senss_sensor_value_header header;
	struct {
		uint32_t timestamp_delta;
		int16_t v;
	} readings[1];
};

/**
 * @brief Sensor value data structure for single 1-axis value.
 * uint16 version
 */
struct senss_sensor_value_uint16 {
	struct senss_sensor_value_header header;
	struct {
		uint32_t timestamp_delta;
		uint16_t v;
	} readings[1];
};

/**
 * @brief Sensor value data structure for single 1-axis value.
 * int8 version
 */
struct senss_sensor_value_int8 {
	struct senss_sensor_value_header header;
	struct {
		uint32_t timestamp_delta;
		int8_t v;
	} readings[1];
};

/**
 * @brief  Sensor value data structure for single 1-axis value.
 * uint8 version
 */
struct senss_sensor_value_uint8 {
	struct senss_sensor_value_header header;
	struct {
		uint32_t timestamp_delta;
		uint8_t v;
	} readings[1];
};

/**
 * @brief Sensor value data structure types based on specific sensor type.
 * suitable for complex sensors, such as human detection.
 */
struct senss_sensor_value_human_detection {
	struct senss_sensor_value_header header;
	struct {
		uint32_t timestamp_delta;
		uint32_t distance;
		uint8_t  confidence;
		uint8_t  presence;
	} readings[1];
};

/**
 * @}
 */

#endif /* SENSS_DATATYPES_H_ */
