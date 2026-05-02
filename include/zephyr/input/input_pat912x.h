/*
 * Copyright 2024 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for PAT912x input driver.
 * @ingroup pat912x_interface
 */

#ifndef ZEPHYR_INCLUDE_INPUT_PAT912X_H_
#define ZEPHYR_INCLUDE_INPUT_PAT912X_H_

/**
 * @defgroup pat912x_interface PAT912x
 * @ingroup input_interface_ext
 * @brief PAT912x Miniature Optical Navigation Chip
 * @{
 */

/**
 * @brief Set resolution on a pat912x device
 *
 * @param dev pat912x device.
 * @param res_x_cpi CPI resolution for the X axis, 0 to 1275, -1 to keep the
 * current value.
 * @param res_y_cpi CPI resolution for the Y axis, 0 to 1275, -1 to keep the
 * current value.
 */
int pat912x_set_resolution(const struct device *dev,
			   int16_t res_x_cpi, int16_t res_y_cpi);

/** @} */

#endif /* ZEPHYR_INCLUDE_INPUT_PAT912X_H_ */
