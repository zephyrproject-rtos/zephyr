/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Decoded data types for the sensor API
 * @ingroup sensor_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_DATA_TYPES_H
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_DATA_TYPES_H

#include <zephyr/dsp/types.h>
#include <zephyr/dsp/print_format.h>

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup sensor_interface
 * @{
 */

/** Header common to all decoded sensor data types. */
struct sensor_data_header {
	/**
	 * The closest timestamp for when the first frame was generated as attained by
	 * k_uptime_ticks().
	 */
	uint64_t base_timestamp_ns;
	/**
	 * The number of elements in the 'readings' array.
	 *
	 * This must be at least 1
	 */
	uint16_t reading_count;
};

/**
 * Data for a sensor channel which reports on three axes. This is used by:
 * - @ref SENSOR_CHAN_ACCEL_XYZ
 * - @ref SENSOR_CHAN_GYRO_XYZ
 * - @ref SENSOR_CHAN_MAGN_XYZ
 * - @ref SENSOR_CHAN_POS_DXYZ
 * - @ref SENSOR_CHAN_GRAVITY_VECTOR
 * - @ref SENSOR_CHAN_GBIAS_XYZ
 */
struct sensor_three_axis_data {
	/** Base timestamp and number of readings. */
	struct sensor_data_header header;
	/**
	 * Scaling of the q31 values in @c readings: a value @c v stands for
	 * v * 2^(shift - 31) in the unit of the channel.
	 */
	int8_t shift;
	/** Reading of a three-axis channel. */
	struct sensor_three_axis_sample_data {
		/** Time of the reading relative to @c header.base_timestamp_ns, in nanoseconds. */
		uint32_t timestamp_delta;
		/** Values, as an array or per axis. */
		union {
			/** X, Y and Z values. */
			q31_t values[3];
			/** Alias of @c values. */
			q31_t v[3];
			/** Values per axis. */
			struct {
				q31_t x; /**< X-axis value. */
				q31_t y; /**< Y-axis value. */
				q31_t z; /**< Z-axis value. */
			};
		};
	} readings[1]; /**< Readings, @c header.reading_count of them. */
};

/**
 * @brief Format string for a reading of a struct sensor_three_axis_data
 *
 * Prints the timestamp of the reading, in nanoseconds, and its X, Y and Z values. Use with
 * @ref PRIsensor_three_axis_data_arg.
 */
#define PRIsensor_three_axis_data PRIu64 "ns, (%" PRIq(6) ", %" PRIq(6) ", %" PRIq(6) ")"

/**
 * @brief Arguments for @ref PRIsensor_three_axis_data
 *
 * @param data_ The struct sensor_three_axis_data to print
 * @param readings_offset_ Index of the reading to print
 */
#define PRIsensor_three_axis_data_arg(data_, readings_offset_)                                     \
	(data_).header.base_timestamp_ns + (data_).readings[(readings_offset_)].timestamp_delta,   \
		PRIq_arg((data_).readings[(readings_offset_)].x, 6, (data_).shift),                \
		PRIq_arg((data_).readings[(readings_offset_)].y, 6, (data_).shift),                \
		PRIq_arg((data_).readings[(readings_offset_)].z, 6, (data_).shift)

/**
 * Data for a sensor channel which reports game rotation vector data. This is used by:
 * - @ref SENSOR_CHAN_GAME_ROTATION_VECTOR
 */
struct sensor_game_rotation_vector_data {
	/** Base timestamp and number of readings. */
	struct sensor_data_header header;
	/**
	 * Scaling of the q31 values in @c readings: a value @c v stands for
	 * v * 2^(shift - 31) in the unit of the channel.
	 */
	int8_t shift;
	/** Reading of a game rotation vector. */
	struct sensor_game_rotation_vector_sample_data {
		/** Time of the reading relative to @c header.base_timestamp_ns, in nanoseconds. */
		uint32_t timestamp_delta;
		/** Unit quaternion components, as an array or by name. */
		union {
			/** X, Y, Z and W components. */
			q31_t values[4];
			/** Alias of @c values. */
			q31_t v[4];
			/** Components by name. */
			struct {
				q31_t x; /**< X component. */
				q31_t y; /**< Y component. */
				q31_t z; /**< Z component. */
				q31_t w; /**< W (scalar) component. */
			};
		};
	} readings[1]; /**< Readings, @c header.reading_count of them. */
};

/**
 * @brief Format string for a reading of a struct sensor_game_rotation_vector_data
 *
 * Prints the timestamp of the reading, in nanoseconds, and its X, Y, Z and W components. Use
 * with @ref PRIsensor_game_rotation_vector_data_arg.
 */
#define PRIsensor_game_rotation_vector_data PRIu64                                                 \
	"ns, (%" PRIq(6) ", %" PRIq(6) ", %" PRIq(6)  ", %" PRIq(6) ")"

/**
 * @brief Arguments for @ref PRIsensor_game_rotation_vector_data
 *
 * @param data_ The struct sensor_game_rotation_vector_data to print
 * @param readings_offset_ Index of the reading to print
 */
#define PRIsensor_game_rotation_vector_data_arg(data_, readings_offset_)                           \
	(data_).header.base_timestamp_ns + (data_).readings[(readings_offset_)].timestamp_delta,   \
		PRIq_arg((data_).readings[(readings_offset_)].x, 6, (data_).shift),                \
		PRIq_arg((data_).readings[(readings_offset_)].y, 6, (data_).shift),                \
		PRIq_arg((data_).readings[(readings_offset_)].z, 6, (data_).shift),                \
		PRIq_arg((data_).readings[(readings_offset_)].w, 6, (data_).shift)

/**
 * Data from a sensor where we only care about an event occurring. This is used to report triggers.
 */
struct sensor_occurrence_data {
	/** Base timestamp and number of readings. */
	struct sensor_data_header header;
	/** Occurrence of an event. */
	struct sensor_occurrence_sample_data {
		/** Time of the event relative to @c header.base_timestamp_ns, in nanoseconds. */
		uint32_t timestamp_delta;
	} readings[1]; /**< Events, @c header.reading_count of them. */
};

/**
 * @brief Format string for an event of a struct sensor_occurrence_data
 *
 * Prints the timestamp of the event, in nanoseconds. Use with
 * @ref PRIsensor_occurrence_data_arg.
 */
#define PRIsensor_occurrence_data PRIu64 "ns"

/**
 * @brief Arguments for @ref PRIsensor_occurrence_data
 *
 * @param data_ The struct sensor_occurrence_data to print
 * @param readings_offset_ Index of the event to print
 */
#define PRIsensor_occurrence_data_arg(data_, readings_offset_)                                     \
	(data_).header.base_timestamp_ns + (data_).readings[(readings_offset_)].timestamp_delta

/**
 * Data for a sensor channel which reports a scalar data.
 */
struct sensor_q31_data {
	/** Base timestamp and number of readings. */
	struct sensor_data_header header;
	/**
	 * Scaling of the q31 values in @c readings: a value @c v stands for
	 * v * 2^(shift - 31) in the unit of the channel.
	 */
	int8_t shift;
	/** Reading of a scalar channel. */
	struct sensor_q31_sample_data {
		/** Time of the reading relative to @c header.base_timestamp_ns, in nanoseconds. */
		uint32_t timestamp_delta;
		/** Value, with one alias per unit. */
		union {
			q31_t value; /**< Value, in the unit of the channel. */
			q31_t light;           /**< Unit: lux */
			q31_t pressure;        /**< Unit: kilopascal */
			q31_t temperature;     /**< Unit: degrees Celsius */
			q31_t percent;         /**< Unit: percent */
			q31_t distance;        /**< Unit: meters */
			q31_t density;         /**< Unit: ug/m^3 */
			q31_t density_ppm;     /**< Unit: parts per million */
			q31_t density_ppb;     /**< Unit: parts per billion */
			q31_t resistance;      /**< Unit: ohms */
			q31_t voltage;         /**< Unit: volts */
			q31_t current;         /**< Unit: amps */
			q31_t power;           /**< Unit: watts */
			q31_t angle;           /**< Unit: degrees */
			q31_t electric_charge; /**< Unit: mAh */
			q31_t humidity;        /**< Unit: RH */
		};
	} readings[1]; /**< Readings, @c header.reading_count of them. */
};

/**
 * @brief Format string for a reading of a struct sensor_q31_data
 *
 * Prints the timestamp of the reading, in nanoseconds, and its value. Use with
 * @ref PRIsensor_q31_data_arg.
 */
#define PRIsensor_q31_data PRIu64 "ns (%" PRIq(6) ")"

/**
 * @brief Arguments for @ref PRIsensor_q31_data
 *
 * @param data_ The struct sensor_q31_data to print
 * @param readings_offset_ Index of the reading to print
 */
#define PRIsensor_q31_data_arg(data_, readings_offset_)                                            \
	(data_).header.base_timestamp_ns + (data_).readings[(readings_offset_)].timestamp_delta,   \
		PRIq_arg((data_).readings[(readings_offset_)].value, 6, (data_).shift)

/**
 * Data from a sensor that produces a byte of data. This is used by:
 * - @ref SENSOR_CHAN_PROX
 */
struct sensor_byte_data {
	/** Base timestamp and number of readings. */
	struct sensor_data_header header;
	/** Reading of a byte channel. */
	struct sensor_byte_sample_data {
		/** Time of the reading relative to @c header.base_timestamp_ns, in nanoseconds. */
		uint32_t timestamp_delta;
		/** Value, as a byte or as flags. */
		union {
			/** Raw value. */
			uint8_t value;
			/** Value as flags. */
			struct {
				uint8_t is_near: 1; /**< 1 if an object is near (proximity). */
				uint8_t padding: 7; /**< Unused. */
			};
		};
	} readings[1]; /**< Readings, @c header.reading_count of them. */
};

/**
 * @brief Format string for a reading of a struct sensor_byte_data
 *
 * Prints the timestamp of the reading, in nanoseconds, and one of its fields. Use with
 * @ref PRIsensor_byte_data_arg.
 *
 * @param field_name_ Field of the reading to print, such as @c value or @c is_near
 */
#define PRIsensor_byte_data(field_name_) PRIu64 "ns (" STRINGIFY(field_name_) " = %" PRIu8 ")"

/**
 * @brief Arguments for @ref PRIsensor_byte_data
 *
 * @param data_ The struct sensor_byte_data to print
 * @param readings_offset_ Index of the reading to print
 * @param field_name_ Field of the reading to print, as passed to @ref PRIsensor_byte_data
 */
#define PRIsensor_byte_data_arg(data_, readings_offset_, field_name_)                              \
	(data_).header.base_timestamp_ns + (data_).readings[(readings_offset_)].timestamp_delta,   \
		(data_).readings[(readings_offset_)].field_name_

/**
 * Data from a sensor that produces a count like value. This is used by:
 * - @ref SENSOR_CHAN_GAUGE_CYCLE_COUNT
 */
struct sensor_uint64_data {
	/** Base timestamp and number of readings. */
	struct sensor_data_header header;
	/** Reading of a count channel. */
	struct sensor_uint64_sample_data {
		/** Time of the reading relative to @c header.base_timestamp_ns, in nanoseconds. */
		uint32_t timestamp_delta;
		/** Count. */
		uint64_t value;
	} readings[1]; /**< Readings, @c header.reading_count of them. */
};

/**
 * @brief Format string for a reading of a struct sensor_uint64_data
 *
 * Prints the timestamp of the reading, in nanoseconds, and its value. Use with
 * @ref PRIsensor_uint64_data_arg.
 */
#define PRIsensor_uint64_data PRIu64 "ns (%" PRIu64 ")"

/**
 * @brief Arguments for @ref PRIsensor_uint64_data
 *
 * @param data_ The struct sensor_uint64_data to print
 * @param readings_offset_ Index of the reading to print
 */
#define PRIsensor_uint64_data_arg(data_, readings_offset_)                                         \
	(data_).header.base_timestamp_ns + (data_).readings[(readings_offset_)].timestamp_delta,   \
		(data_).readings[(readings_offset_)].value

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_DATA_TYPES_H */
