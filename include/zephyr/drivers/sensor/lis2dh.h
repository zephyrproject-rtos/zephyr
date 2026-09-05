/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of LIS2DH sensor
 * @ingroup lis2dh_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_LIS2DH_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_LIS2DH_H_

/**
 * @defgroup lis2dh_interface LIS2DH
 * @ingroup sensor_interface_ext_st
 * @brief ST Microelectronics LIS2DH 3-axis accelerometer
 * @{
 */

#include <zephyr/drivers/sensor.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief A single LIS2DH FIFO sample.
 */
struct lis2dh_fifo_sample {
	/** Acceleration along X, Y and Z, in m/s^2. */
	struct sensor_value accel[3];
	/** Estimated sample time in nanoseconds since boot. */
	uint64_t timestamp_ns;
};

/**
 * @brief Start hardware FIFO streaming.
 *
 * Requires CONFIG_LIS2DH_FIFO. Use from thread context with a nonzero ODR.
 * The watermark is configured by the fifo-watermark devicetree property.
 * This interface and sensor_stream() are alternative FIFO consumers.
 *
 * @param dev LIS2DH device.
 *
 * @retval 0 FIFO streaming started.
 * @retval -EBUSY INT1 or FIFO streaming is already in use.
 * @retval -EBUSY Cleanup from a previous failure is still pending.
 * @retval -ENOTSUP An INT1 GPIO is unavailable.
 * @retval -EINVAL The current ODR/power-mode combination cannot stream.
 * @retval <0 Bus or GPIO error.
 */
int lis2dh_fifo_start(const struct device *dev);

/**
 * @brief Stop hardware FIFO streaming and discard queued samples.
 *
 * After a bus or GPIO error, call again to retry cleanup. New starts and
 * register configuration are rejected until cleanup succeeds.
 * Requires CONFIG_LIS2DH_FIFO and thread context. This also stops an active
 * sensor_stream() request and completes its pending submission.
 *
 * @param dev LIS2DH device.
 *
 * @retval 0 FIFO streaming stopped.
 * @retval <0 Bus or GPIO error.
 */
int lis2dh_fifo_stop(const struct device *dev);

/**
 * @brief Read samples drained from the hardware FIFO.
 *
 * Requires CONFIG_LIS2DH_FIFO and thread context. This call does not wait
 * for data: an empty queue returns zero samples. Use with lis2dh_fifo_start();
 * sensor_stream() delivers its samples through RTIO, not this queue.
 *
 * @param dev LIS2DH device.
 * @param samples Caller-owned destination, valid for this call. May be NULL
 *                only when @p capacity is zero.
 * @param capacity Maximum number of entries to copy to @p samples.
 * @param count Non-NULL output for the number of samples copied on success.
 *
 * @retval 0 Samples copied successfully.
 * @retval -EACCES FIFO streaming is not active.
 * @retval -EINVAL A required argument is NULL.
 */
int lis2dh_fifo_read(const struct device *dev, struct lis2dh_fifo_sample *samples, size_t capacity,
		     size_t *count);

/**
 * Possible values for @ref SENSOR_ATTR_LIS2DH_SELF_TEST custom attribute.
 */
enum lis2dh_self_test {
	LIS2DH_SELF_TEST_DISABLE = 0,  /**< Self-test disabled */
	LIS2DH_SELF_TEST_POSITIVE = 1, /**< Simulates a positive-direction acceleration */
	LIS2DH_SELF_TEST_NEGATIVE = 2, /**< Simulates a negative-direction acceleration */
};

/**
 * @brief Custom sensor attributes for LIS2DH
 */
enum sensor_attribute_lis2dh {
	/**
	 * Sets the self-test mode.
	 *
	 * Applies an electrostatic force to the sensor to simulate acceleration without
	 * actually moving the device.
	 *
	 * Use a value from @ref lis2dh_self_test, passed in the sensor_value.val1 field.
	 */
	SENSOR_ATTR_LIS2DH_SELF_TEST = SENSOR_ATTR_PRIV_START,
	/**
	 * Lifetime software queue overwrite count, saturating at INT32_MAX.
	 * Requires CONFIG_LIS2DH_FIFO_STATS. Does not count hardware losses
	 * or RTIO delivery losses; start/stop/drop do not reset the count.
	 */
	SENSOR_ATTR_LIS2DH_FIFO_DROPPED,
};

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_LIS2DH_H_ */
