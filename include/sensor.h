/* sensor.h - public sensor driver API */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __SENSOR_H__
#define __SENSOR_H__

/**
 * @brief Sensor Interface
 * @defgroup sensor_interface Sensor Interface
 * @ingroup io_interfaces
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <device.h>
#include <errno.h>

/** @brief Sensor value types. */
enum sensor_value_type {
	/** val1 contains an integer value, val2 is unused. */
	SENSOR_TYPE_INT,
	/**
	 * val1 contains an integer value, val2 is the fractional value.
	 * To obtain the final value, use the formula: val1 + val2 *
	 * 10^(-6).
	 */
	SENSOR_TYPE_INT_PLUS_MICRO,
	/**
	 * @brief val1 contains a Q16.16 representation, val2 is
	 * unused.
	 */
	SENSOR_TYPE_Q16_16,
	/** @brief dval contains a floating point value. */
	SENSOR_TYPE_DOUBLE,
};

/**
 * @brief Representation of a sensor readout value.
 *
 * The meaning of the fields is dictated by the type field.
 */
struct sensor_value {
	enum sensor_value_type type;
	union {
		struct {
			int32_t val1;
			int32_t val2;
		};
		double dval;
	};
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
	/** Acceleration on any axis. */
	SENSOR_CHAN_ACCEL_ANY,
	/** Angular velocity around the X axis, in radians/s. */
	SENSOR_CHAN_GYRO_X,
	/** Angular velocity around the Y axis, in radians/s. */
	SENSOR_CHAN_GYRO_Y,
	/** Angular velocity around the Z axis, in radians/s. */
	SENSOR_CHAN_GYRO_Z,
	/** Angular velocity on any axis. */
	SENSOR_CHAN_GYRO_ANY,
	/** Magnetic field on the X axis, in Gauss. */
	SENSOR_CHAN_MAGN_X,
	/** Magnetic field on the Y axis, in Gauss. */
	SENSOR_CHAN_MAGN_Y,
	/** Magnetic field on the Z axis, in Gauss. */
	SENSOR_CHAN_MAGN_Z,
	/** Magnetic field on any axis. */
	SENSOR_CHAN_MAGN_ANY,
	/** Temperature in degrees Celsius. */
	SENSOR_CHAN_TEMP,
	/** Pressure in kilopascal. */
	SENSOR_CHAN_PRESS,
	/**
	 * Proximity.  Adimensional.  A value of 1 indicates that an
	 * object is close.
	 */
	SENSOR_CHAN_PROX,
	/** Humidity, in milli percent. */
	SENSOR_CHAN_HUMIDITY,
	/** Illuminance in visible spectrum, in lux. */
	SENSOR_CHAN_LIGHT,
	/** Illuminance in infra-red spectrum, in lux. */
	SENSOR_CHAN_IR,
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
	 * acceleration of gyro. If detection is based on slope between
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
	 * SENSOR_ATTR_LOWER_THRESH and @ref SENSOR_ATTR_UPPER_THRESH
	 * attributes.
	 */
	SENSOR_TRIG_THRESHOLD,
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
	/** Oversampling factor */
	SENSOR_ATTR_OVERSAMPLING,
};

typedef void (*sensor_trigger_handler_t)(struct device *dev,
					 struct sensor_trigger *trigger);

typedef int (*sensor_attr_set_t)(struct device *dev,
				 enum sensor_channel chan,
				 enum sensor_attribute attr,
				 const struct sensor_value *val);
typedef int (*sensor_trigger_set_t)(struct device *dev,
				    const struct sensor_trigger *trig,
				    sensor_trigger_handler_t handler);
typedef int (*sensor_sample_fetch_t)(struct device *dev);
typedef int (*sensor_channel_get_t)(struct device *dev,
				    enum sensor_channel chan,
				    struct sensor_value *val);

struct sensor_driver_api {
	sensor_attr_set_t attr_set;
	sensor_trigger_set_t trigger_set;
	sensor_sample_fetch_t sample_fetch;
	sensor_channel_get_t channel_get;
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
static inline int sensor_attr_set(struct device *dev,
				  enum sensor_channel chan,
				  enum sensor_attribute attr,
				  const struct sensor_value *val)
{
	struct sensor_driver_api *api;

	api = (struct sensor_driver_api *)dev->driver_api;
	if (!api->attr_set) {
		return -ENOTSUP;
	}

	return api->attr_set(dev, chan, attr, val);
}

/**
 * @brief Activate a sensor's trigger and set the trigger handler
 *
 * The handler will be called from a fiber, so I2C or SPI operations are
 * safe.  However, the fiber's stack is limited and defined by the
 * driver.  It is currently up to the caller to ensure that the handler
 * does not overflow the stack.
 *
 * @param dev Pointer to the sensor device
 * @param trig The trigger to activate
 * @param handler The function that should be called when the trigger
 * fires
 *
 * @return 0 if successful, negative errno code if failure.
 */
static inline int sensor_trigger_set(struct device *dev,
				     struct sensor_trigger *trig,
				     sensor_trigger_handler_t handler)
{
	struct sensor_driver_api *api;

	api = (struct sensor_driver_api *)dev->driver_api;
	if (!api->trigger_set) {
		return -ENOTSUP;
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
 * Since the function communicates with the sensor device, it is unsafe
 * to call it in an ISR if the device is connected via I2C or SPI.
 *
 * @param dev Pointer to the sensor device
 *
 * @return 0 if successful, negative errno code if failure.
 */
static inline int sensor_sample_fetch(struct device *dev)
{
	struct sensor_driver_api *api;

	api = (struct sensor_driver_api *)dev->driver_api;

	return api->sample_fetch(dev);
}

/**
 * @brief Get a reading from a sensor device
 *
 * Return a useful value for a particular channel, from the driver's
 * internal data.  Before calling this function, a sample must be
 * obtained by calling @ref sensor_sample_fetch.  It is guaranteed that
 * two subsequent calls of this function for the same channels will
 * yield the same value, if @ref sensor_sample_fetch has not been called
 * in the meantime.
 *
 * @param dev Pointer to the sensor device
 * @param chan The channel to read
 * @param val Where to store the value
 *
 * @return 0 if successful, negative errno code if failure.
 */
static inline int sensor_channel_get(struct device *dev,
				     enum sensor_channel chan,
				     struct sensor_value *val)
{
	struct sensor_driver_api *api;

	api = (struct sensor_driver_api *)dev->driver_api;

	return api->channel_get(dev, chan, val);
}

#ifdef CONFIG_SENSOR_DELAYED_WORK
typedef void (*sensor_work_handler_t)(void *arg);

/**
 * @brief Sensor delayed work descriptor.
 *
 * Used by sensor drivers internally to delay function calls to a fiber
 * context.
 */
struct sensor_work {
	sensor_work_handler_t handler;
	void *arg;
};

/**
 * @brief Get a fifo to which sensor delayed work can be submitted
 *
 * If @ref CONFIG_SENSOR_DELAYED_WORK is enabled, the system creates a
 * global fiber that can execute delayed work on behalf of drivers.
 * This is useful for drivers which need a mechanism of delayed work but
 * do not create their own fibers due to system resource constraints.
 */
struct nano_fifo *sensor_get_work_fifo(void);
#endif

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __SENSOR_H__ */
