/**
 * @file drivers/sensor.h
 *
 * @brief Public APIs for the sensor driver.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_H_

/**
 * @brief Sensor Interface
 * @defgroup sensor_interface Sensor Interface
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <stdlib.h>

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/dsp/types.h>
#include <zephyr/rtio/rtio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Representation of a sensor readout value.
 *
 * The value is represented as having an integer and a fractional part,
 * and can be obtained using the formula val1 + val2 * 10^(-6). Negative
 * values also adhere to the above formula, but may need special attention.
 * Here are some examples of the value representation:
 *
 *      0.5: val1 =  0, val2 =  500000
 *     -0.5: val1 =  0, val2 = -500000
 *     -1.0: val1 = -1, val2 =  0
 *     -1.5: val1 = -1, val2 = -500000
 */
struct sensor_value {
	/** Integer part of the value. */
	int32_t val1;
	/** Fractional part of the value (in one-millionth parts). */
	int32_t val2;
};

/**
 * @brief Sensor channels.
 */
enum sensor_channel {
	/** Acceleration on the X axis, in m/s^2. */
	SENSOR_CHAN_ACCEL_X,
	/** Acceleration on the Y axis, in m/s^2. */
	SENSOR_CHAN_ACCEL_Y,
	/** Acceleration on the Z axis, in m/s^2. */
	SENSOR_CHAN_ACCEL_Z,
	/** Acceleration on the X, Y and Z axes. */
	SENSOR_CHAN_ACCEL_XYZ,
	/** Angular velocity around the X axis, in radians/s. */
	SENSOR_CHAN_GYRO_X,
	/** Angular velocity around the Y axis, in radians/s. */
	SENSOR_CHAN_GYRO_Y,
	/** Angular velocity around the Z axis, in radians/s. */
	SENSOR_CHAN_GYRO_Z,
	/** Angular velocity around the X, Y and Z axes. */
	SENSOR_CHAN_GYRO_XYZ,
	/** Magnetic field on the X axis, in Gauss. */
	SENSOR_CHAN_MAGN_X,
	/** Magnetic field on the Y axis, in Gauss. */
	SENSOR_CHAN_MAGN_Y,
	/** Magnetic field on the Z axis, in Gauss. */
	SENSOR_CHAN_MAGN_Z,
	/** Magnetic field on the X, Y and Z axes. */
	SENSOR_CHAN_MAGN_XYZ,
	/** Device die temperature in degrees Celsius. */
	SENSOR_CHAN_DIE_TEMP,
	/** Ambient temperature in degrees Celsius. */
	SENSOR_CHAN_AMBIENT_TEMP,
	/** Pressure in kilopascal. */
	SENSOR_CHAN_PRESS,
	/**
	 * Proximity.  Adimensional.  A value of 1 indicates that an
	 * object is close.
	 */
	SENSOR_CHAN_PROX,
	/** Humidity, in percent. */
	SENSOR_CHAN_HUMIDITY,
	/** Illuminance in visible spectrum, in lux. */
	SENSOR_CHAN_LIGHT,
	/** Illuminance in infra-red spectrum, in lux. */
	SENSOR_CHAN_IR,
	/** Illuminance in red spectrum, in lux. */
	SENSOR_CHAN_RED,
	/** Illuminance in green spectrum, in lux. */
	SENSOR_CHAN_GREEN,
	/** Illuminance in blue spectrum, in lux. */
	SENSOR_CHAN_BLUE,
	/** Altitude, in meters */
	SENSOR_CHAN_ALTITUDE,

	/** 1.0 micro-meters Particulate Matter, in ug/m^3 */
	SENSOR_CHAN_PM_1_0,
	/** 2.5 micro-meters Particulate Matter, in ug/m^3 */
	SENSOR_CHAN_PM_2_5,
	/** 10 micro-meters Particulate Matter, in ug/m^3 */
	SENSOR_CHAN_PM_10,
	/** Distance. From sensor to target, in meters */
	SENSOR_CHAN_DISTANCE,

	/** CO2 level, in parts per million (ppm) **/
	SENSOR_CHAN_CO2,
	/** VOC level, in parts per billion (ppb) **/
	SENSOR_CHAN_VOC,
	/** Gas sensor resistance in ohms. */
	SENSOR_CHAN_GAS_RES,

	/** Voltage, in volts **/
	SENSOR_CHAN_VOLTAGE,
	/** Current, in amps **/
	SENSOR_CHAN_CURRENT,
	/** Power in watts **/
	SENSOR_CHAN_POWER,

	/** Resistance , in Ohm **/
	SENSOR_CHAN_RESISTANCE,

	/** Angular rotation, in degrees */
	SENSOR_CHAN_ROTATION,

	/** Position change on the X axis, in points. */
	SENSOR_CHAN_POS_DX,
	/** Position change on the Y axis, in points. */
	SENSOR_CHAN_POS_DY,
	/** Position change on the Z axis, in points. */
	SENSOR_CHAN_POS_DZ,

	/** Revolutions per minute, in RPM. */
	SENSOR_CHAN_RPM,

	/** Voltage, in volts **/
	SENSOR_CHAN_GAUGE_VOLTAGE,
	/** Average current, in amps **/
	SENSOR_CHAN_GAUGE_AVG_CURRENT,
	/** Standby current, in amps **/
	SENSOR_CHAN_GAUGE_STDBY_CURRENT,
	/** Max load current, in amps **/
	SENSOR_CHAN_GAUGE_MAX_LOAD_CURRENT,
	/** Gauge temperature  **/
	SENSOR_CHAN_GAUGE_TEMP,
	/** State of charge measurement in % **/
	SENSOR_CHAN_GAUGE_STATE_OF_CHARGE,
	/** Full Charge Capacity in mAh **/
	SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY,
	/** Remaining Charge Capacity in mAh **/
	SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY,
	/** Nominal Available Capacity in mAh **/
	SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY,
	/** Full Available Capacity in mAh **/
	SENSOR_CHAN_GAUGE_FULL_AVAIL_CAPACITY,
	/** Average power in mW **/
	SENSOR_CHAN_GAUGE_AVG_POWER,
	/** State of health measurement in % **/
	SENSOR_CHAN_GAUGE_STATE_OF_HEALTH,
	/** Time to empty in minutes **/
	SENSOR_CHAN_GAUGE_TIME_TO_EMPTY,
	/** Time to full in minutes **/
	SENSOR_CHAN_GAUGE_TIME_TO_FULL,
	/** Cycle count (total number of charge/discharge cycles) **/
	SENSOR_CHAN_GAUGE_CYCLE_COUNT,
	/** Design voltage of cell in V (max voltage)*/
	SENSOR_CHAN_GAUGE_DESIGN_VOLTAGE,
	/** Desired voltage of cell in V (nominal voltage) */
	SENSOR_CHAN_GAUGE_DESIRED_VOLTAGE,
	/** Desired charging current in mA */
	SENSOR_CHAN_GAUGE_DESIRED_CHARGING_CURRENT,

	/** All channels. */
	SENSOR_CHAN_ALL,

	/**
	 * Number of all common sensor channels.
	 */
	SENSOR_CHAN_COMMON_COUNT,

	/**
	 * This and higher values are sensor specific.
	 * Refer to the sensor header file.
	 */
	SENSOR_CHAN_PRIV_START = SENSOR_CHAN_COMMON_COUNT,

	/**
	 * Maximum value describing a sensor channel type.
	 */
	SENSOR_CHAN_MAX = INT16_MAX,
};

/**
 * @brief Sensor trigger types.
 */
enum sensor_trigger_type {
	/**
	 * Timer-based trigger, useful when the sensor does not have an
	 * interrupt line.
	 */
	SENSOR_TRIG_TIMER,
	/** Trigger fires whenever new data is ready. */
	SENSOR_TRIG_DATA_READY,
	/**
	 * Trigger fires when the selected channel varies significantly.
	 * This includes any-motion detection when the channel is
	 * acceleration or gyro. If detection is based on slope between
	 * successive channel readings, the slope threshold is configured
	 * via the @ref SENSOR_ATTR_SLOPE_TH and @ref SENSOR_ATTR_SLOPE_DUR
	 * attributes.
	 */
	SENSOR_TRIG_DELTA,
	/** Trigger fires when a near/far event is detected. */
	SENSOR_TRIG_NEAR_FAR,
	/**
	 * Trigger fires when channel reading transitions configured
	 * thresholds.  The thresholds are configured via the @ref
	 * SENSOR_ATTR_LOWER_THRESH, @ref SENSOR_ATTR_UPPER_THRESH, and
	 * @ref SENSOR_ATTR_HYSTERESIS attributes.
	 */
	SENSOR_TRIG_THRESHOLD,

	/** Trigger fires when a single tap is detected. */
	SENSOR_TRIG_TAP,

	/** Trigger fires when a double tap is detected. */
	SENSOR_TRIG_DOUBLE_TAP,

	/** Trigger fires when a free fall is detected. */
	SENSOR_TRIG_FREEFALL,

	/** Trigger fires when motion is detected. */
	SENSOR_TRIG_MOTION,

	/** Trigger fires when no motion has been detected for a while. */
	SENSOR_TRIG_STATIONARY,
	/**
	 * Number of all common sensor triggers.
	 */
	SENSOR_TRIG_COMMON_COUNT,

	/**
	 * This and higher values are sensor specific.
	 * Refer to the sensor header file.
	 */
	SENSOR_TRIG_PRIV_START = SENSOR_TRIG_COMMON_COUNT,

	/**
	 * Maximum value describing a sensor trigger type.
	 */
	SENSOR_TRIG_MAX = INT16_MAX,
};

/**
 * @brief Sensor trigger spec.
 */
struct sensor_trigger {
	/** Trigger type. */
	enum sensor_trigger_type type;
	/** Channel the trigger is set on. */
	enum sensor_channel chan;
};

/**
 * @brief Sensor attribute types.
 */
enum sensor_attribute {
	/**
	 * Sensor sampling frequency, i.e. how many times a second the
	 * sensor takes a measurement.
	 */
	SENSOR_ATTR_SAMPLING_FREQUENCY,
	/** Lower threshold for trigger. */
	SENSOR_ATTR_LOWER_THRESH,
	/** Upper threshold for trigger. */
	SENSOR_ATTR_UPPER_THRESH,
	/** Threshold for any-motion (slope) trigger. */
	SENSOR_ATTR_SLOPE_TH,
	/**
	 * Duration for which the slope values needs to be
	 * outside the threshold for the trigger to fire.
	 */
	SENSOR_ATTR_SLOPE_DUR,
	/* Hysteresis for trigger thresholds. */
	SENSOR_ATTR_HYSTERESIS,
	/** Oversampling factor */
	SENSOR_ATTR_OVERSAMPLING,
	/** Sensor range, in SI units. */
	SENSOR_ATTR_FULL_SCALE,
	/**
	 * The sensor value returned will be altered by the amount indicated by
	 * offset: final_value = sensor_value + offset.
	 */
	SENSOR_ATTR_OFFSET,
	/**
	 * Calibration target. This will be used by the internal chip's
	 * algorithms to calibrate itself on a certain axis, or all of them.
	 */
	SENSOR_ATTR_CALIB_TARGET,
	/** Configure the operating modes of a sensor. */
	SENSOR_ATTR_CONFIGURATION,
	/** Set a calibration value needed by a sensor. */
	SENSOR_ATTR_CALIBRATION,
	/** Enable/disable sensor features */
	SENSOR_ATTR_FEATURE_MASK,
	/** Alert threshold or alert enable/disable */
	SENSOR_ATTR_ALERT,
	/** Free-fall duration represented in milliseconds.
	 *  If the sampling frequency is changed during runtime,
	 *  this attribute should be set to adjust freefall duration
	 *  to the new sampling frequency.
	 */
	SENSOR_ATTR_FF_DUR,
	/**
	 * Number of all common sensor attributes.
	 */
	SENSOR_ATTR_COMMON_COUNT,

	/**
	 * This and higher values are sensor specific.
	 * Refer to the sensor header file.
	 */
	SENSOR_ATTR_PRIV_START = SENSOR_ATTR_COMMON_COUNT,

	/**
	 * Maximum value describing a sensor attribute type.
	 */
	SENSOR_ATTR_MAX = INT16_MAX,
};

/**
 * @typedef sensor_trigger_handler_t
 * @brief Callback API upon firing of a trigger
 *
 * @param dev Pointer to the sensor device
 * @param trigger The trigger
 */
typedef void (*sensor_trigger_handler_t)(const struct device *dev,
					 const struct sensor_trigger *trigger);

/**
 * @typedef sensor_attr_set_t
 * @brief Callback API upon setting a sensor's attributes
 *
 * See sensor_attr_set() for argument description
 */
typedef int (*sensor_attr_set_t)(const struct device *dev,
				 enum sensor_channel chan,
				 enum sensor_attribute attr,
				 const struct sensor_value *val);

/**
 * @typedef sensor_attr_get_t
 * @brief Callback API upon getting a sensor's attributes
 *
 * See sensor_attr_get() for argument description
 */
typedef int (*sensor_attr_get_t)(const struct device *dev,
				 enum sensor_channel chan,
				 enum sensor_attribute attr,
				 struct sensor_value *val);

/**
 * @typedef sensor_trigger_set_t
 * @brief Callback API for setting a sensor's trigger and handler
 *
 * See sensor_trigger_set() for argument description
 */
typedef int (*sensor_trigger_set_t)(const struct device *dev,
				    const struct sensor_trigger *trig,
				    sensor_trigger_handler_t handler);
/**
 * @typedef sensor_sample_fetch_t
 * @brief Callback API for fetching data from a sensor
 *
 * See sensor_sample_fetch() for argument description
 */
typedef int (*sensor_sample_fetch_t)(const struct device *dev,
				     enum sensor_channel chan);
/**
 * @typedef sensor_channel_get_t
 * @brief Callback API for getting a reading from a sensor
 *
 * See sensor_channel_get() for argument description
 */
typedef int (*sensor_channel_get_t)(const struct device *dev,
				    enum sensor_channel chan,
				    struct sensor_value *val);

/**
 * @typedef sensor_frame_iterator_t
 * @brief Used for iterating over the data frames via the :c:struct:`sensor_decoder_api`
 *
 * Example usage:
 *
 * @code(.c)
 *     sensor_frame_iterator_t fit = {0}, fit_last;
 *     sensor_channel_iterator_t cit = {0}, cit_last;
 *
 *     while (true) {
 *       int num_decoded_channels;
 *       enum sensor_channel channel;
 *       q31_t value;
 *
 *       fit_last = fit;
 *       num_decoded_channels = decoder->decode(buffer, &fit, &cit, &channel, &value, 1);
 *
 *       if (num_decoded_channels <= 0) {
 *         printk("Done decoding buffer\n");
 *         break;
 *       }
 *
 *       printk("Decoded channel (%d) with value %s0.%06" PRIi64 "\n", q < 0 ? "-" : "",
 *              abs(q) * INT64_C(1000000) / (INT64_C(1) << 31));
 *
 *       if (fit_last != fit) {
 *         printk("Finished decoding frame\n");
 *       }
 *     }
 * @endcode
 */
typedef uint32_t sensor_frame_iterator_t;

/**
 * @typedef sensor_channel_iterator_t
 * @brief Used for iterating over data channels in the same frame via :c:struct:`sensor_decoder_api`
 */
typedef uint32_t sensor_channel_iterator_t;

/**
 * @brief Decodes a single raw data buffer
 *
 * Data buffers are provided on the :c:struct:`rtio` context that's supplied to
 * c:func:`sensor_read`.
 */
struct sensor_decoder_api {
	/**
	 * @brief Get the number of frames in the current buffer.
	 *
	 * @param[in]  buffer The buffer provided on the :c:struct:`rtio` context.
	 * @param[out] frame_count The number of frames on the buffer (at least 1)
	 * @return 0 on success
	 * @return <0 on error
	 */
	int (*get_frame_count)(const uint8_t *buffer, uint16_t *frame_count);

	/**
	 * @brief Get the timestamp associated with the first frame.
	 *
	 * @param[in]  buffer The buffer provided on the :c:struct:`rtio` context.
	 * @param[out] timestamp_ns The closest timestamp for when the first frame was generated
	 *             as attained by :c:func:`k_uptime_ticks`.
	 * @return 0 on success
	 * @return <0 on error
	 */
	int (*get_timestamp)(const uint8_t *buffer, uint64_t *timestamp_ns);

	/**
	 * @brief Get the shift count of a particular channel (multiplier)
	 *
	 * This value can be used by shifting the q31_t value resulting in the SI unit of the
	 * reading. It is guaranteed that the shift for a channel will not change between frames.
	 *
	 * @param[in]  buffer The buffer provided on the :c:struct:`rtio` context.
	 * @param[in]  channel_type The c:enum:`sensor_channel` to query
	 * @param[out] shift The bit shift of the channel for this data buffer.
	 * @return 0 on success
	 * @return -EINVAL if the @p channel_type doesn't exist in the buffer
	 * @return <0 on error
	 */
	int (*get_shift)(const uint8_t *buffer, enum sensor_channel channel_type, int8_t *shift);

	/**
	 * @brief Decode up to N samples from the buffer
	 *
	 * This function will never wrap frames. If 1 channel is available in the current frame and
	 * @p max_count is 2, only 1 channel will be decoded and the frame iterator will be modified
	 * so that the next call to decode will begin at the next frame.
	 *
	 * @param[in]     buffer The buffer provided on the :c:struct:`rtio` context
	 * @param[in,out] fit The current frame iterator
	 * @param[in,out] cit The current channel iterator
	 * @param[out]    channels The channels that were decoded
	 * @param[out]    values The scaled data that was decoded
	 * @param[in]     max_count The maximum number of channels to decode.
	 * @return
	 */
	int (*decode)(const uint8_t *buffer, sensor_frame_iterator_t *fit,
		      sensor_channel_iterator_t *cit, enum sensor_channel *channels, q31_t *values,
		      uint8_t max_count);
};

/**
 * @typedef sensor_get_decoder_t
 * @brief Get the decoder associate with the given device
 *
 * @see sensor_get_decoder for more details
 */
typedef int (*sensor_get_decoder_t)(const struct device *dev,
				    struct sensor_decoder_api *api);

/*
 * Internal data structure used to store information about the IODevice for async reading and
 * streaming sensor data.
 */
struct sensor_read_config {
	const struct device *sensor;
	enum sensor_channel *const channels;
	size_t num_channels;
	const size_t max_channels;
};

/**
 * @brief Define a reading instance of a sensor
 *
 * Use this macro to generate a :c:struct:`rtio_iodev` for reading specific channels. Example:
 *
 * @code(.c)
 * SENSOR_DT_READ_IODEV(icm42688_accelgyro, DT_NODELABEL(icm42688),
 *     SENSOR_CHAN_ACCEL_XYZ, SENSOR_CHAN_GYRO_XYZ);
 *
 * int main(void) {
 *   sensor_read(&icm42688_accelgyro, &rtio);
 * }
 * @endcode
 */
#define SENSOR_DT_READ_IODEV(name, dt_node, ...)                                                   \
	static enum sensor_channel __channel_array_##name[] = {__VA_ARGS__};                       \
	static struct sensor_read_config __sensor_read_config_##name = {                           \
		.sensor = DEVICE_DT_GET(dt_node),                                                  \
		.channels = __channel_array_##name,                                                \
		.num_channels = ARRAY_SIZE(__channel_array_##name),                                \
		.max_channels = ARRAY_SIZE(__channel_array_##name),                                \
	};                                                                                         \
	RTIO_IODEV_DEFINE(name, &__sensor_iodev_api, &__sensor_read_config_##name)

/* Used to submit an RTIO sqe to the sensor's iodev */
typedef int (*sensor_submit_t)(const struct device *sensor, struct rtio_iodev_sqe *sqe);

/* The default decoder API */
extern struct sensor_decoder_api __sensor_default_decoder;

/* The default sensor iodev API */
extern struct rtio_iodev_api __sensor_iodev_api;

__subsystem struct sensor_driver_api {
	sensor_attr_set_t attr_set;
	sensor_attr_get_t attr_get;
	sensor_trigger_set_t trigger_set;
	sensor_sample_fetch_t sample_fetch;
	sensor_channel_get_t channel_get;
	sensor_get_decoder_t get_decoder;
	sensor_submit_t submit;
};

/**
 * @brief Set an attribute for a sensor
 *
 * @param dev Pointer to the sensor device
 * @param chan The channel the attribute belongs to, if any.  Some
 * attributes may only be set for all channels of a device, depending on
 * device capabilities.
 * @param attr The attribute to set
 * @param val The value to set the attribute to
 *
 * @return 0 if successful, negative errno code if failure.
 */
__syscall int sensor_attr_set(const struct device *dev,
			      enum sensor_channel chan,
			      enum sensor_attribute attr,
			      const struct sensor_value *val);

static inline int z_impl_sensor_attr_set(const struct device *dev,
					 enum sensor_channel chan,
					 enum sensor_attribute attr,
					 const struct sensor_value *val)
{
	const struct sensor_driver_api *api =
		(const struct sensor_driver_api *)dev->api;

	if (api->attr_set == NULL) {
		return -ENOSYS;
	}

	return api->attr_set(dev, chan, attr, val);
}

/**
 * @brief Get an attribute for a sensor
 *
 * @param dev Pointer to the sensor device
 * @param chan The channel the attribute belongs to, if any.  Some
 * attributes may only be set for all channels of a device, depending on
 * device capabilities.
 * @param attr The attribute to get
 * @param val Pointer to where to store the attribute
 *
 * @return 0 if successful, negative errno code if failure.
 */
__syscall int sensor_attr_get(const struct device *dev,
			      enum sensor_channel chan,
			      enum sensor_attribute attr,
			      struct sensor_value *val);

static inline int z_impl_sensor_attr_get(const struct device *dev,
					 enum sensor_channel chan,
					 enum sensor_attribute attr,
					 struct sensor_value *val)
{
	const struct sensor_driver_api *api =
		(const struct sensor_driver_api *)dev->api;

	if (api->attr_get == NULL) {
		return -ENOSYS;
	}

	return api->attr_get(dev, chan, attr, val);
}

/**
 * @brief Activate a sensor's trigger and set the trigger handler
 *
 * The handler will be called from a thread, so I2C or SPI operations are
 * safe.  However, the thread's stack is limited and defined by the
 * driver.  It is currently up to the caller to ensure that the handler
 * does not overflow the stack.
 *
 * The user-allocated trigger will be stored by the driver as a pointer, rather
 * than a copy, and passed back to the handler. This enables the handler to use
 * CONTAINER_OF to retrieve a context pointer when the trigger is embedded in a
 * larger struct and requires that the trigger is not allocated on the stack.
 *
 * @funcprops \supervisor
 *
 * @param dev Pointer to the sensor device
 * @param trig The trigger to activate
 * @param handler The function that should be called when the trigger
 * fires
 *
 * @return 0 if successful, negative errno code if failure.
 */
static inline int sensor_trigger_set(const struct device *dev,
				     const struct sensor_trigger *trig,
				     sensor_trigger_handler_t handler)
{
	const struct sensor_driver_api *api =
		(const struct sensor_driver_api *)dev->api;

	if (api->trigger_set == NULL) {
		return -ENOSYS;
	}

	return api->trigger_set(dev, trig, handler);
}

/**
 * @brief Fetch a sample from the sensor and store it in an internal
 * driver buffer
 *
 * Read all of a sensor's active channels and, if necessary, perform any
 * additional operations necessary to make the values useful.  The user
 * may then get individual channel values by calling @ref
 * sensor_channel_get.
 *
 * The function blocks until the fetch operation is complete.
 *
 * Since the function communicates with the sensor device, it is unsafe
 * to call it in an ISR if the device is connected via I2C or SPI.
 *
 * @param dev Pointer to the sensor device
 *
 * @return 0 if successful, negative errno code if failure.
 */
__syscall int sensor_sample_fetch(const struct device *dev);

static inline int z_impl_sensor_sample_fetch(const struct device *dev)
{
	const struct sensor_driver_api *api =
		(const struct sensor_driver_api *)dev->api;

	return api->sample_fetch(dev, SENSOR_CHAN_ALL);
}

/**
 * @brief Fetch a sample from the sensor and store it in an internal
 * driver buffer
 *
 * Read and compute compensation for one type of sensor data (magnetometer,
 * accelerometer, etc). The user may then get individual channel values by
 * calling @ref sensor_channel_get.
 *
 * This is mostly implemented by multi function devices enabling reading at
 * different sampling rates.
 *
 * The function blocks until the fetch operation is complete.
 *
 * Since the function communicates with the sensor device, it is unsafe
 * to call it in an ISR if the device is connected via I2C or SPI.
 *
 * @param dev Pointer to the sensor device
 * @param type The channel that needs updated
 *
 * @return 0 if successful, negative errno code if failure.
 */
__syscall int sensor_sample_fetch_chan(const struct device *dev,
				       enum sensor_channel type);

static inline int z_impl_sensor_sample_fetch_chan(const struct device *dev,
						  enum sensor_channel type)
{
	const struct sensor_driver_api *api =
		(const struct sensor_driver_api *)dev->api;

	return api->sample_fetch(dev, type);
}

/**
 * @brief Get a reading from a sensor device
 *
 * Return a useful value for a particular channel, from the driver's
 * internal data.  Before calling this function, a sample must be
 * obtained by calling @ref sensor_sample_fetch or
 * @ref sensor_sample_fetch_chan. It is guaranteed that two subsequent
 * calls of this function for the same channels will yield the same
 * value, if @ref sensor_sample_fetch or @ref sensor_sample_fetch_chan
 * has not been called in the meantime.
 *
 * For vectorial data samples you can request all axes in just one call
 * by passing the specific channel with _XYZ suffix. The sample will be
 * returned at val[0], val[1] and val[2] (X, Y and Z in that order).
 *
 * @param dev Pointer to the sensor device
 * @param chan The channel to read
 * @param val Where to store the value
 *
 * @return 0 if successful, negative errno code if failure.
 */
__syscall int sensor_channel_get(const struct device *dev,
				 enum sensor_channel chan,
				 struct sensor_value *val);

static inline int z_impl_sensor_channel_get(const struct device *dev,
					    enum sensor_channel chan,
					    struct sensor_value *val)
{
	const struct sensor_driver_api *api =
		(const struct sensor_driver_api *)dev->api;

	return api->channel_get(dev, chan, val);
}

/*
 * Generic data structure used for encoding sensor channel information and corresponding scale.
 */
struct __attribute__((__packed__)) sensor_data_generic_channel {
	int8_t shift;
	enum sensor_channel channel;
};

/*
 * Generic data structure used for encoding the sample timestamp and number of channels sampled.
 */
struct __attribute__((__packed__)) sensor_data_generic_header {
	uint64_t timestamp_ns;
	size_t num_channels;
	struct sensor_data_generic_channel channel_info[0];
};

/**
 * @brief checks if a given channel is a 3-axis channel
 *
 * @param[in] chan The channel to check
 * @return true if @p chan is :c:enum:`SENSOR_CHAN_ACCEL_XYZ`
 * @return true if @p chan is :c:enum:`SENSOR_CHAN_GYRO_XYZ`
 * @return true if @p chan is :c:enum:`SENSOR_CHAN_MAGN_XYZ`
 * @return false otherwise
 */
#define SENSOR_CHANNEL_3_AXIS(chan)                                                                \
	((chan) == SENSOR_CHAN_ACCEL_XYZ || (chan) == SENSOR_CHAN_GYRO_XYZ ||                      \
	 (chan) == SENSOR_CHAN_MAGN_XYZ)

/**
 * @brief Get the sensor's decoder API
 *
 * @param[in] dev The sensor device
 * @param[in] decoder Pointer to the decoder which will be set upon success
 * @return 0 on success
 * @return < 0 on error
 */
__syscall int sensor_get_decoder(const struct device *dev,
				 struct sensor_decoder_api *decoder);

static inline int z_impl_sensor_get_decoder(const struct device *dev,
					    struct sensor_decoder_api *decoder)
{
	const struct sensor_driver_api *api = (const struct sensor_driver_api *)dev->api;

	__ASSERT_NO_MSG(api != NULL);

	if (api->get_decoder == NULL) {
		*decoder = __sensor_default_decoder;
		return 0;
	}

	return api->get_decoder(dev, decoder);
}

/**
 * @brief Reconfigure a reading iodev
 *
 * Allows a reconfiguration of the iodev associated with reading a sample from a sensor.
 *
 * <b>Important</b>: If the iodev is currently servicing a read operation, the data will likely be
 * invalid. Please be sure the flush or wait for all pending operations to complete before using the
 * data after a configuration change.
 *
 * It is also important that the `data` field of the iodev is a :c:struct:`sensor_read_config`.
 *
 * @param[in] iodev The iodev to reconfigure
 * @param[in] sensor The sensor to read from
 * @param[in] channels One or more channels to read
 * @param[in] num_channels The number of channels in @p channels
 * @return 0 on success
 * @return < 0 on error
 */
__syscall int sensor_reconfigure_read_iodev(struct rtio_iodev *iodev, const struct device *sensor,
					    const enum sensor_channel *channels,
					    size_t num_channels);

static inline int z_impl_sensor_reconfigure_read_iodev(struct rtio_iodev *iodev,
						       const struct device *sensor,
						       const enum sensor_channel *channels,
						       size_t num_channels)
{
	struct sensor_read_config *cfg = (struct sensor_read_config *)iodev->data;

	if (cfg->max_channels < num_channels) {
		return -ENOMEM;
	}

	cfg->sensor = sensor;
	memcpy(cfg->channels, channels, num_channels * sizeof(enum sensor_channel));
	cfg->num_channels = num_channels;
	return 0;
}

/**
 * @brief Read data from a sensor.
 *
 * Using @p cfg, read one snapshot of data from the device by using the provided RTIO context
 * @p ctx. This call will generate a :c:struct:`rtio_sqe` that will leverage the RTIO's internal
 * mempool when the time comes to service the read.
 *
 * @param[in] iodev The iodev created by :c:macro:`SENSOR_DT_READ_IODEV`
 * @param[in] ctx The RTIO context to service the read
 * @param[in] userdata Optional userdata that will be available when the read is complete
 * @return 0 on success
 * @return < 0 on error
 */
static inline int sensor_read(struct rtio_iodev *iodev, struct rtio *ctx, void *userdata)
{
	if (IS_ENABLED(CONFIG_USERSPACE)) {
		struct rtio_sqe sqe;

		rtio_sqe_prep_read_with_pool(&sqe, iodev, 0, userdata);
		rtio_sqe_copy_in(ctx, &sqe, 1);
	} else {
		struct rtio_sqe *sqe = rtio_sqe_acquire(ctx);

		if (sqe == NULL) {
			return -ENOMEM;
		}
		rtio_sqe_prep_read_with_pool(sqe, iodev, 0, userdata);
	}
	rtio_submit(ctx, 0);
	return 0;
}

/**
 * @typedef sensor_processing_callback_t
 * @brief Callback function used with the helper processing loop :c:func:`z_sensor_processing_loop`
 *
 * @param[in] result The result code of the read (0 being success)
 * @param[in] buf The data buffer holding the sensor data
 * @param[in] buf_len The length (in bytes) of the @p buf
 * @param[in] userdata The optional userdata passed to :c:func:`sensor_read`
 */
typedef void (*sensor_processing_callback_t)(int result, uint8_t *buf, uint32_t buf_len,
					     void *userdata);

/**
 * @brief Helper function for common processing of sensor data.
 *
 * This function can be called in a blocking manner after :c:func:`sensor_read` or in a standalone
 * thread dedicated to processing. It will wait for a cqe from the RTIO context, once received, it
 * will decode the userdata and call the @p cb. Once the @p cb returns, the buffer will be released
 * back into @p ctx's mempool if available.
 *
 * @param[in] ctx The RTIO context to wait on
 * @param[in] cb Callback to call when data is ready for processing
 */
void z_sensor_processing_loop(struct rtio *ctx, sensor_processing_callback_t cb);

/**
 * @brief The value of gravitational constant in micro m/s^2.
 */
#define SENSOR_G		9806650LL

/**
 * @brief The value of constant PI in micros.
 */
#define SENSOR_PI		3141592LL

/**
 * @brief Helper function to convert acceleration from m/s^2 to Gs
 *
 * @param ms2 A pointer to a sensor_value struct holding the acceleration,
 *            in m/s^2.
 *
 * @return The converted value, in Gs.
 */
static inline int32_t sensor_ms2_to_g(const struct sensor_value *ms2)
{
	int64_t micro_ms2 = ms2->val1 * 1000000LL + ms2->val2;

	if (micro_ms2 > 0) {
		return (micro_ms2 + SENSOR_G / 2) / SENSOR_G;
	} else {
		return (micro_ms2 - SENSOR_G / 2) / SENSOR_G;
	}
}

/**
 * @brief Helper function to convert acceleration from Gs to m/s^2
 *
 * @param g The G value to be converted.
 * @param ms2 A pointer to a sensor_value struct, where the result is stored.
 */
static inline void sensor_g_to_ms2(int32_t g, struct sensor_value *ms2)
{
	ms2->val1 = ((int64_t)g * SENSOR_G) / 1000000LL;
	ms2->val2 = ((int64_t)g * SENSOR_G) % 1000000LL;
}

/**
 * @brief Helper function to convert acceleration from m/s^2 to micro Gs
 *
 * @param ms2 A pointer to a sensor_value struct holding the acceleration,
 *            in m/s^2.
 *
 * @return The converted value, in micro Gs.
 */
static inline int32_t sensor_ms2_to_ug(const struct sensor_value *ms2)
{
	int64_t micro_ms2 = (ms2->val1 * INT64_C(1000000)) + ms2->val2;

	return (micro_ms2 * 1000000LL) / SENSOR_G;
}

/**
 * @brief Helper function to convert acceleration from micro Gs to m/s^2
 *
 * @param ug The micro G value to be converted.
 * @param ms2 A pointer to a sensor_value struct, where the result is stored.
 */
static inline void sensor_ug_to_ms2(int32_t ug, struct sensor_value *ms2)
{
	ms2->val1 = ((int64_t)ug * SENSOR_G / 1000000LL) / 1000000LL;
	ms2->val2 = ((int64_t)ug * SENSOR_G / 1000000LL) % 1000000LL;
}

/**
 * @brief Helper function for converting radians to degrees.
 *
 * @param rad A pointer to a sensor_value struct, holding the value in radians.
 *
 * @return The converted value, in degrees.
 */
static inline int32_t sensor_rad_to_degrees(const struct sensor_value *rad)
{
	int64_t micro_rad_s = rad->val1 * 1000000LL + rad->val2;

	if (micro_rad_s > 0) {
		return (micro_rad_s * 180LL + SENSOR_PI / 2) / SENSOR_PI;
	} else {
		return (micro_rad_s * 180LL - SENSOR_PI / 2) / SENSOR_PI;
	}
}

/**
 * @brief Helper function for converting degrees to radians.
 *
 * @param d The value (in degrees) to be converted.
 * @param rad A pointer to a sensor_value struct, where the result is stored.
 */
static inline void sensor_degrees_to_rad(int32_t d, struct sensor_value *rad)
{
	rad->val1 = ((int64_t)d * SENSOR_PI / 180LL) / 1000000LL;
	rad->val2 = ((int64_t)d * SENSOR_PI / 180LL) % 1000000LL;
}

/**
 * @brief Helper function for converting radians to 10 micro degrees.
 *
 * When the unit is 1 micro degree, the range that the int32_t can represent is
 * +/-2147.483 degrees. In order to increase this range, here we use 10 micro
 * degrees as the unit.
 *
 * @param rad A pointer to a sensor_value struct, holding the value in radians.
 *
 * @return The converted value, in 10 micro degrees.
 */
static inline int32_t sensor_rad_to_10udegrees(const struct sensor_value *rad)
{
	int64_t micro_rad_s = rad->val1 * 1000000LL + rad->val2;

	return (micro_rad_s * 180LL * 100000LL) / SENSOR_PI;
}

/**
 * @brief Helper function for converting 10 micro degrees to radians.
 *
 * @param d The value (in 10 micro degrees) to be converted.
 * @param rad A pointer to a sensor_value struct, where the result is stored.
 */
static inline void sensor_10udegrees_to_rad(int32_t d, struct sensor_value *rad)
{
	rad->val1 = ((int64_t)d * SENSOR_PI / 180LL / 100000LL) / 1000000LL;
	rad->val2 = ((int64_t)d * SENSOR_PI / 180LL / 100000LL) % 1000000LL;
}

/**
 * @brief Helper function for converting struct sensor_value to double.
 *
 * @param val A pointer to a sensor_value struct.
 * @return The converted value.
 */
static inline double sensor_value_to_double(const struct sensor_value *val)
{
	return (double)val->val1 + (double)val->val2 / 1000000;
}

/**
 * @brief Helper function for converting double to struct sensor_value.
 *
 * @param val A pointer to a sensor_value struct.
 * @param inp The converted value.
 * @return 0 if successful, negative errno code if failure.
 */
static inline int sensor_value_from_double(struct sensor_value *val, double inp)
{
	if (inp < INT32_MIN || inp > INT32_MAX) {
		return -ERANGE;
	}

	double val2 = (inp - (int32_t)inp) * 1000000.0;

	if (val2 < INT32_MIN || val2 > INT32_MAX) {
		return -ERANGE;
	}

	val->val1 = (int32_t)inp;
	val->val2 = (int32_t)val2;

	return 0;
}

#ifdef CONFIG_SENSOR_INFO

struct sensor_info {
	const struct device *dev;
	const char *vendor;
	const char *model;
	const char *friendly_name;
};

#define SENSOR_INFO_INITIALIZER(_dev, _vendor, _model, _friendly_name)	\
	{								\
		.dev = _dev,						\
		.vendor = _vendor,					\
		.model = _model,					\
		.friendly_name = _friendly_name,			\
	}

#define SENSOR_INFO_DEFINE(name, ...)					\
	static const STRUCT_SECTION_ITERABLE(sensor_info, name) =	\
		SENSOR_INFO_INITIALIZER(__VA_ARGS__)

#define SENSOR_INFO_DT_NAME(node_id)					\
	_CONCAT(__sensor_info, DEVICE_DT_NAME_GET(node_id))

#define SENSOR_INFO_DT_DEFINE(node_id)					\
	SENSOR_INFO_DEFINE(SENSOR_INFO_DT_NAME(node_id),		\
			   DEVICE_DT_GET(node_id),			\
			   DT_NODE_VENDOR_OR(node_id, NULL),		\
			   DT_NODE_MODEL_OR(node_id, NULL),		\
			   DT_PROP_OR(node_id, friendly_name, NULL))	\

#else

#define SENSOR_INFO_DEFINE(name, ...)
#define SENSOR_INFO_DT_DEFINE(node_id)

#endif /* CONFIG_SENSOR_INFO */

/**
 * @brief Like DEVICE_DT_DEFINE() with sensor specifics.
 *
 * @details Defines a device which implements the sensor API. May define an
 * element in the sensor info iterable section used to enumerate all sensor
 * devices.
 *
 * @param node_id The devicetree node identifier.
 *
 * @param init_fn Name of the init function of the driver.
 *
 * @param pm_device PM device resources reference (NULL if device does not use
 * PM).
 *
 * @param data_ptr Pointer to the device's private data.
 *
 * @param cfg_ptr The address to the structure containing the configuration
 * information for this instance of the driver.
 *
 * @param level The initialization level. See SYS_INIT() for details.
 *
 * @param prio Priority within the selected initialization level. See
 * SYS_INIT() for details.
 *
 * @param api_ptr Provides an initial pointer to the API function struct used
 * by the driver. Can be NULL.
 */
#define SENSOR_DEVICE_DT_DEFINE(node_id, init_fn, pm_device,		\
				data_ptr, cfg_ptr, level, prio,		\
				api_ptr, ...)				\
	DEVICE_DT_DEFINE(node_id, init_fn, pm_device,			\
			 data_ptr, cfg_ptr, level, prio,		\
			 api_ptr, __VA_ARGS__);				\
									\
	SENSOR_INFO_DT_DEFINE(node_id);

/**
 * @brief Like SENSOR_DEVICE_DT_DEFINE() for an instance of a DT_DRV_COMPAT
 * compatible
 *
 * @param inst instance number. This is replaced by
 * <tt>DT_DRV_COMPAT(inst)</tt> in the call to SENSOR_DEVICE_DT_DEFINE().
 *
 * @param ... other parameters as expected by SENSOR_DEVICE_DT_DEFINE().
 */
#define SENSOR_DEVICE_DT_INST_DEFINE(inst, ...)				\
	SENSOR_DEVICE_DT_DEFINE(DT_DRV_INST(inst), __VA_ARGS__)

/**
 * @brief Helper function for converting struct sensor_value to integer milli units.
 *
 * @param val A pointer to a sensor_value struct.
 * @return The converted value.
 */
static inline int64_t sensor_value_to_milli(struct sensor_value *val)
{
	return ((int64_t)val->val1 * 1000) + val->val2 / 1000;
}

/**
 * @brief Helper function for converting struct sensor_value to integer micro units.
 *
 * @param val A pointer to a sensor_value struct.
 * @return The converted value.
 */
static inline int64_t sensor_value_to_micro(struct sensor_value *val)
{
	return ((int64_t)val->val1 * 1000000) + val->val2;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/sensor.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_H_ */
