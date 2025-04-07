/*
 * Copyright (c) 2025 COGNID Telematik GmbH
 */

#ifndef _SENSOR_SIM_ASYNC_H_
#define _SENSOR_SIM_ASYNC_H_

/**
 * @file sensor_sim_async.h
 *
 * @brief Public API for the sensor sim async driver.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include <zephyr/drivers/sensor.h>

struct sensor_sim_async_sensor_fifo_sample {
	union {
		struct {
			/* All values in 1/SENSOR_SIM_ASYNC_SCALE
			 * 16-bit integers are chosen to save flash if we run on limited hardware
			 */
			int16_t x;
			int16_t y;
			int16_t z;
		};
		int16_t val[3];
	};
} __aligned(sizeof(int16_t));

/**
 * Sets a fallback value that will be the default decoded value, if the
 * requested decode channel is not the fifo channel previously set in
 * sensor_sim_async_feed_data(). This allows to add a constant value like
 * temperature to the encoded data that can be decoded later.
 *
 * @param dev Pointer to the device structure.
 * @param value Fallback value to set.
 */
void sensor_sim_async_set_fallback_value(const struct device *dev, float value);

/**
 * Feed data into the simulator which is later output by the sensor stream (or
 * fetch + get). Timestamps are adjusted automatically based on ODR. The
 * timestamp for the first value may be set using start_ns. Only one channel can
 * be encoded into the simulated sensor FIFO. To output a constant value on
 * another channel in the stream, use sensor_sim_async_set_fallback_value().
 *
 * @param dev Sensor sim device instance
 * @param test_data Test data to feed into sensor
 * @param count Number of test data items
 * @param start_ns -1 to not set a new timestamp, >=0 to set timestamp of first
 * test data value
 * @param chan Sensor channel
 */
void sensor_sim_async_feed_data(const struct device *dev,
				const struct sensor_sim_async_sensor_fifo_sample test_data[],
				uint32_t count, int64_t start_ns, enum sensor_channel chan);

/**
 * Manually trigger some sensor trigger. The trigger will
 *	- lead to a callback, if there is one set
 *	- end up in the encoded data to be then extracted using has_trigger()
 *
 * @param dev Device instance
 * @param trigger Trigger to trigger
 */
void sensor_sim_async_trigger(const struct device *dev, enum sensor_trigger_type trigger);

/**
 * Flush the simulated internal sensor FIFO from values that may not yet have
 * been streamed out.
 *
 * @param dev Device instance
 */
void sensor_sim_async_flush_fifo(const struct device *dev);

/**
 * Sets the value of a specified sensor channel.
 *
 * @param dev Pointer to the device structure.
 * @param chan Enum value representing the sensor channel to be set.
 * @param val Value to be set for the specified sensor channel.
 *
 * @return 0 if the value is set successfully, -EINVAL if the channel is
 * invalid.
 */
int sensor_sim_async_set_channel(const struct device *dev, enum sensor_channel chan, float val);

#ifdef __cplusplus
}
#endif

#endif /* _SENSOR_SIM_ASYNC_H_ */
