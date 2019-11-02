/*
 * Copyright (c) 2019  Tom Burdick <tom.burdick@electromatic.us>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RTIO_SENSOR_H_
#define ZEPHYR_INCLUDE_DRIVERS_RTIO_SENSOR_H_
/**
 * @brief RTIO Sensor Interface
 * @defgroup rtio_sensor_interface RTIO Sensor Interface
 * @ingroup io_interfaces
 * @{
 */

#include <kernel.h>
#include <device.h>
#include <drivers/rtio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief Real-Time IO Sensor API
 *
 * Each RTIO Sensor acts as a pollable stream of buffers with a driver that
 * provides a way of deserializing the raw sensor data buffers into meaningful
 * values.
 *
 * Each RTIO Sensor is expected to output buffers containing sample sets (such
 * as 9 axis IMU data) in a device specific format understood by the driver.
 * Each driver must then implement the sensor reader api and functionality
 * to deserialize this stream of buffers into meaningful SI unit data. The
 * caller may request different numerical formats of this data.
 *
 * Sample sets may contain 0 or more sensor channel values. Channels are
 * described by their type and their index such that a 3 axis accelerometer
 * has 3 channels of acceleration (x,y,z). Common aliases for the index for
 * XYZ are provided for convienence and to ensure commonality among drivers.
 *
 * A sample set may vary in what it contains, even in the same block.
 * Some devices allow for different sampling rates of different channels.
 *
 * It must be understood by the user of this API what devices they are using
 * and what channels may or may not be available at any given time.
 *
 * For each new rtio_block received from the sensor, an rtio_sensor_reader
 * should be re-initialized. This allows the driver to provide an optimal
 * reader for the rtio_block avoiding unnecessary branching when possible.
 *
 * *Note* To start this API provides 32bit floating point numerical formatted
 * values. Q format values with scale factors would be very useful for
 * use with CMSIS DSP or many SoCs that do not contain FPUs. The reader
 * *should* provide simple ways of converting integer readings into
 * useful numerical formats in common functions.
 */

/**
 * @private
 */
struct rtio_sensor_reader;

/**
 * @private
 */
struct rtio_sensor_channel;

/**
 * @private
 * @brief Read a value from the sensor reader into the channel value
 *
 * @retval 0 Value unavailable
 * @retval 1 Value read
 */
typedef int (*rtio_sensor_channel_read)(
	const struct rtio_sensor_reader *reader,
	struct rtio_sensor_channel *channel);


/**
 * @brief Sensor Channel Type
 *
 * This describes the kind of measurement being done.
 *
 * This does not describe the axis, units, or other parts of the measurement.
 * Only what it is.
 *
 * This is always extensible outside of zephyr by external drivers by using
 * enumerations after the marker value describing the last type.
 */
enum rtio_sensor_channel_type {
	RTIO_SENSOR_CHAN_TIMESTAMP,
	RTIO_SENSOR_CHAN_ACCELERATION,
	RTIO_SENSOR_CHAN_ANGULAR_VELOCITY,
	RTIO_SENSOR_CHAN_MAGNETIC_FIELD,
	RTIO_SENSOR_CHAN_TEMPERATURE,
	RTIO_SENSOR_CHAN_PRESSURE,
	RTIO_SENSOR_CHAN_HUMIDITY,
	RTIO_SENSOR_CHAN_ALTITUDE,
	RTIO_SENSOR_CHAN_DISTANCE,
	RTIO_SENSOR_CHAN_LIGHT,
	RTIO_SENSOR_CHAN_PARTICULATE_MATTER,

	/* Volatile Organic Compounds */
	RTIO_SENSOR_CHAN_VOC,

	/**
	 * Number of all common sensor channels.
	 */
	RTIO_SENSOR_CHAN_COMMON_COUNT,

	/**
	 * This and higher values are sensor specific.
	 * Refer to the sensor header file.
	 */
	RITO_SENSOR_CHAN_PRIV_START = RTIO_SENSOR_CHAN_COMMON_COUNT,

	/**
	 * Maximum value describing a sensor channel type.
	 */
	RTIO_SENSOR_CHAN_MAX = INT16_MAX,
};

/**
 * Numerical format
 */
enum rtio_sensor_numerical_format {
	RTIO_SENSOR_NUM_F32,

	/**
	 * Number of all common numerical formats.
	 */
	RTIO_SENSOR_NUM_COMMON_COUNT,

	/**
	 * This and higher values are sensor specific.
	 * Refer to the sensor header file.
	 */
	RITO_SENSOR_NUM_PRIV_START = RTIO_SENSOR_NUM_COMMON_COUNT,

	/**
	 * Maximum value describing a sensor channel type.
	 */
	RTIO_SENSOR_NUM_MAX = INT16_MAX,
};


/**
 * @define Common X Axis channel index
 */
#define RTIO_SENSOR_X_AXIS_INDEX 0

/**
 * @define Common Y Axis channel index
 */
#define RTIO_SENSOR_Y_AXIS_INDEX 1

/**
 * @define Common Z Axis channel index
 *
#define RTIO_SENSOR_Z_AXIS_INDEX 2

/**
 * @define Common Die Temperature channel index
 */
#define RTIO_SENSOR_DIE_INDEX 0

/**
 * @define Common Ambient Temperature channel index
 */
#define RTIO_SENSOR_AMBIENT_INDEX 0

/**
 * @brief RTIO Sensor Channel
 *
 * Each sensor channel can identified by the unique pair (type, index) per
 * device. This is to allow for scenarios of *multiple* channels of similiarly
 * typed measurements such as ADC's sampling voltage, spectrometers sampling
 * light, or other such devices. Common indexes defined above are useful for
 * 3 axis accelerometer, gyro, and magnetometer type devices.
 *
 * The struct itself is used by the reader when iterating over sample sets
 * to determine what channels of the sensor to read from the buffer and
 * into what numerical formats.
 */
struct rtio_sensor_channel {
	/**
	 * Physical or virtual type of sensor channel such as timestamp,
	 * acceleration, rotational speed, magnetic field measurement,
	 * temperature, humidity, light, and so on.
	 *
	 * This describes the what of the channel you'd like to select.
	 */
	enum rtio_sensor_channel_type type;

	/**
	 * In cases where a sensor has multiple channels of the same type such
	 * as a spectrometer type sensors where there might be N channels of
	 * light at different wavelengths, capacitive sensors with multiple
	 * points such as those coming from the TI FDC 2214 this secondary
	 * index gives a reference to which of the sensor channels
	 * to select for a given type.
	 *
	 * For 3 axis accelerometers/gyros/magnetometers this is also used
	 * to describe the Axis.
	 *
	 * This describes the which of the channel you'd like to select.
	 */
	size_t channel_type_idx;

	/**
	 * Read status code (what was returned by rtio_channel_read)
	 *
	 * 1 If the value is valid
	 * 0 If the value was not read
	 */
	int status;

	/**
	 * Numerical format to return the sensor value in
	 */
	enum rtio_sensor_numerical_format format;

	/**
	 * Numerical value for the sensor value
	 */
	union {
		float f32;
	} value;

	/**
	 * @private
	 * A function pointer which takes all the information about
	 * where, what, which, and into what format to read.
	 */
	rtio_sensor_channel_read_t _read_fn;
}

/**
 * @private
 * @brief Get the number of sample sets in the current block
 *
 * @retval -EINVAL invalid reader
 * @retval 0 or greater Number of sample sets to iterate over
 */
typedef ssize_t (*rtio_sensor_reader_sample_sets_t)(
	const struct rtio_sensor_reader *reader);

/**
 * @private
 * @brief Iterate to the next sample set in the block
 *
 * The first call to next on an iterator will do *nothing* as
 * its already at the 0th position. Instead it will replace the
 * noop func with a true next func on its first call.
 *
 * This allows for a straight forward while loop to be created.
 *
 * @retval 0 Complete
 * @retval 1 More to read
 */
typedef int (*rtio_sensor_reader_next_t)(struct rtio_sensor_reader *reader);

/**
 * @brief RTIO Sensor Reader
 */
struct rtio_sensor_reader {
	rtio_sensor_sample_sets_t sample_sets;
	rtio_sensor_reader_next_t next;

	rtio_sensor_channel *channels;
	size_t num_channels;

	struct rtio_block *block;
	size_t position;
};

/**
 * @brief Get the number of sample sets in a sensor reader
 *
 * The reader is only valid after it has been initialized by the device driver
 * with a valid block and until it has finished reading the block.
 *
 * @param reader Pointer to a valid rtio_sensor_reader.
 *
 * @retval -EINVAL Invalid sensor reader
 * @retval 0 or greater Number of sample sets
 */
ssize_t rtio_sensor_reader_sample_sets(const struct rtio_sensor_reader *reader)
{
	if (reader->block == NULL) {
		return -EINVAL;
	}
	return reader->sample_sets(reader);
}

/**
 * @brief Iterate to the next sample set
 *
 * The first call to this does nothing but is required as the initial
 * state is a no-op call. After the first call this moves
 * the position of the reader to the index into the buffer where
 * the next sample set lies until the last sample set is read.
 *
 * If the next sample set is beyond the end of the buffer this should
 * return 0 and set its *block to NULL to remain invalid after.
 *
 * @param reader Pointer to a valid rtio_sensor_reader.
 *
 * @retval -EINVAL Invalid reader
 * @retval 0 No more to read
 * @retval 1 More to read
 */
int rtio_sensor_reader_next(struct rtio_sensor_reader *reader)
{
	if (reader->block == NULL) {
		return -EINVAL;
	}
	return reader->next(reader);
}

/**
 * @private
 * @brief Initialize a block reader with the given device and block
 *
 * The caller is still responsible for freeing the rtio_block.
 *
 * @param dev Pointer to a valid RTIO sensor device
 * @param block Pointer to a valid struct rtio_block
 * @param reader Pointer to a valid struct rtio_sensor_reader
 * @param channels Pointer to a valid array of struct rtio_sensor_channel values
 * @param num_channels Number of values in array of struct rtio_sensor_channel
 *        values
 *
 * @retval 0 Success
 * @retval -EINVAL Invalid parameter
 */
typedef int (*rtio_sensor_reader_t)(struct device *dev,
				    struct rtio_block *block,
				    struct rtio_sensor_reader *reader,
				    struct rtio_sensor_channel *channels,
				    size_t num_channels);

/**
 * @brief Real-Time IO Sensor Driver API
 *
 * This extends the RTIO device API with sensor specific functionality.
 *
 * Specifically this API *requires* a sensor provide a way of initializing
 * a sensor reader based on the rtio_sensor device, rtio_block, and an
 * array of sensor channels to be read out.
 */
struct rtio_sensor_api {
	/** Configuration function pointer *must* be implemented */
	struct rtio_api rtio_api;

	/** Function to initialize RTIO Block Reader using this device and a block  */
	rtio_sensor_reader_t sensor_reader;
};


/**
 * @brief Initialize a block reader with the given device and block
 *
 * The purpose of this call is to allow for distinct statically created
 * rtio_sensor_reader functions to exist for various configurations a sensor
 * may be in. This avoids the need, potentially, of having many branch
 * statements in the reader itself which is expected to be called
 * much more frequently than this function.
 *
 * The caller is still responsible for freeing the rtio_block.
 *
 * @param dev A valid pointer to an instance of device that implements rtio_sensor_device
 * @param block A valid pointer to an rtio_block instance
 * @param reader A pointer to a rtio_sensor_reader which is initialize on success
 * @param channels Pointer to a valid array of struct rtio_sensor_channel values
 * @param num_channels Number of values in array of struct rtio_sensor_channel
 *        values
 *
 * @retval 0 Success
 * @retval -EINVAL Invalid parameter
 */
int rtio_sensor_reader(struct device *dev,
		       struct rtio_block *block,
		       struct rtio_sensor_reader *reader,
		       struct rtio_sensor_channel *channels,
		       size_t num_channels)
{
	const struct rtio_sensor_api *api =
		(const struct rtio_sensor_api *)dev->driver_api;

	return api->sensor_reader(dev, block, reader, channels, num_channels);
}


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_RTIO_SENSOR_H_ */
