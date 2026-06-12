/*
 * Copyright (c) 2025 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of BMM350 sensor
 * @ingroup bmm350_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_BMM350_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_BMM350_H_

/**
 * @brief Bosch BMM350 3-Axis Magnetometer
 * @defgroup bmm350_interface BMM350
 * @ingroup sensor_interface_ext
 * @{
 */

#include <zephyr/device.h>

/**
 * @brief BMM350 user self-test result.
 *
 * Holds the magnetic response, in micro-Tesla, to the positive and negative
 * user self-test excitation on the X and Y axes, together with the resulting
 * per-axis delta (positive - negative). A healthy sensor produces a delta well
 * above the device's self-test limit on both axes; see the BMM350 datasheet for
 * the expected magnitude.
 */
struct bmm350_self_test_result {
	int32_t x_pos;   /**< X-axis response to positive excitation [uT] */
	int32_t x_neg;   /**< X-axis response to negative excitation [uT] */
	int32_t y_pos;   /**< Y-axis response to positive excitation [uT] */
	int32_t y_neg;   /**< Y-axis response to negative excitation [uT] */
	int32_t x_delta; /**< x_pos - x_neg [uT] */
	int32_t y_delta; /**< y_pos - y_neg [uT] */
};

/**
 * @brief Run the BMM350 built-in user self-test.
 *
 * Re-initialises the TMR sensor and measures the response to a positive and
 * negative excitation on the X and Y axes, following the Bosch SensorAPI
 * sequence. The device configuration in place before the call is restored on
 * return.
 *
 * The caller is responsible for judging the result against the device's
 * self-test limit (see the BMM350 datasheet); this function only reports the
 * measured responses.
 *
 * This must not be called while the sensor is streaming.
 *
 * @param dev    Pointer to the sensor device.
 * @param result Output: per-axis self-test responses and deltas.
 *
 * @return 0 if successful, negative errno code on failure.
 */
int bmm350_self_test(const struct device *dev, struct bmm350_self_test_result *result);

/**
 * @brief Performs a magnetic reset.
 *
 * The BMM350 has measures to recover from excessively strong
 * magnetic fields. A magnetic reset is triggered after reset by
 * the device itself, but if excessive field exposure has occurred
 * in suspend mode, the sensor can not detect that event. If a
 * large offset or sensitivity drift indicates that such an event
 * has occurred, then this function can be called by the
 * application.
 *
 * "Enhanced" Magnetic Reset supported by later BMM350 Revisions
 * is not yet implemented.
 *
 * @param dev Pointer to the sensor device
 *
 * @return 0 if successful, negative errno code if failure.
 */
int bmm350_magnetic_reset(const struct device *dev);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_BMM350_H_ */
