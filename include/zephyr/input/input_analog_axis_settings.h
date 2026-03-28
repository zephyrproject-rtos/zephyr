/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public header file for API allowing to save analog axis calibration data.
 * @ingroup input_analog_axis
 */

#ifndef ZEPHYR_INCLUDE_INPUT_ANALOG_AXIS_SETTINGS_H_
#define ZEPHYR_INCLUDE_INPUT_ANALOG_AXIS_SETTINGS_H_

#include <stdint.h>
#include <zephyr/device.h>

/**
 * @addtogroup input_analog_axis
 * @{
 */

/**
 * @brief Save the calibration data.
 *
 * Save the calibration data permanently on the specified device, requires
 * the @ref settings subsystem to be configured and initialized.
 *
 * @param dev Analog axis device.
 *
 * @retval 0 If successful.
 * @retval -errno In case of any other error.
 */
int analog_axis_calibration_save(const struct device *dev);

/** @} */

#endif /* ZEPHYR_INCLUDE_INPUT_ANALOG_AXIS_SETTINGS_H_ */
