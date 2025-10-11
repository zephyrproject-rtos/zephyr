/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup input_analog_axis
 * @brief Main header file for interacting with analog axis input devices.
 */

#ifndef ZEPHYR_INCLUDE_INPUT_ANALOG_AXIS_H_
#define ZEPHYR_INCLUDE_INPUT_ANALOG_AXIS_H_

#include <stdint.h>
#include <zephyr/device.h>

/**
 * @defgroup input_analog_axis Analog axis
 * @ingroup input_interface
 * @{
 */

/**
 * @brief Analog axis calibration data structure.
 *
 * Holds the calibration data for a single analog axis. Initial values are set
 * from the devicetree and can be changed by the application in runtime using
 * @ref analog_axis_calibration_set and @ref analog_axis_calibration_get.
 */
struct analog_axis_calibration {
	/** Input value that corresponds to the minimum output value. */
	int16_t in_min;
	/** Input value that corresponds to the maximum output value. */
	int16_t in_max;
	/** Input value center deadzone. */
	uint16_t in_deadzone;
};

/**
 * @brief Analog axis raw data callback.
 *
 * @param dev Analog axis device.
 * @param channel Channel number.
 * @param raw_val Raw value for the channel.
 */
typedef void (*analog_axis_raw_data_t)(const struct device *dev,
				       int channel, int16_t raw_val);

/**
 * @brief Set a raw data callback.
 *
 * Set a callback to receive raw data for the specified analog axis device.
 * This is meant to be use in the application to acquire the data to use for
 * calibration. Set cb to NULL to disable the callback.
 *
 * @param dev Analog axis device.
 * @param cb An analog_axis_raw_data_t callback to use, NULL to disable.
 */
void analog_axis_set_raw_data_cb(const struct device *dev, analog_axis_raw_data_t cb);

/**
 * @brief Get the number of defined axes.
 *
 * @param dev Analog axis device.
 * @retval n The number of defined axes for dev.
 */
int analog_axis_num_axes(const struct device *dev);

/**
 * @brief Get the axis calibration data.
 *
 * @param dev Analog axis device.
 * @param channel Channel number.
 * @param cal Pointer to an analog_axis_calibration structure that is going to
 * get set with the current calibration data.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If the specified channel is not valid.
 */
int analog_axis_calibration_get(const struct device *dev,
				int channel,
				struct analog_axis_calibration *cal);

/**
 * @brief Set the axis calibration data.
 *
 * @param dev Analog axis device.
 * @param channel Channel number.
 * @param cal Pointer to an analog_axis_calibration structure with the new
 * calibration data
 *
 * @retval 0 If successful.
 * @retval -EINVAL If the specified channel is not valid.
 */
int analog_axis_calibration_set(const struct device *dev,
				int channel,
				struct analog_axis_calibration *cal);

/** @} */

#endif /* ZEPHYR_INCLUDE_INPUT_ANALOG_AXIS_H_ */
