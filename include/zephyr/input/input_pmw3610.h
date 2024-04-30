/*
 * Copyright 2024 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_INPUT_PMW3610_H_
#define ZEPHYR_INCLUDE_INPUT_PMW3610_H_

/**
 * @brief Set resolution on a pmw3610 device
 *
 * @param dev pmw3610 device.
 * @param res_cpi CPI resolution, 200 to 3200.
 */
int pmw3610_set_resolution(const struct device *dev, uint16_t res_cpi);

/**
 * @brief Set force awake mode on a pmw3610 device
 *
 * @param dev pmw3610 device.
 * @param enable whether to enable or disable force awake mode.
 */
int pmw3610_force_awake(const struct device *dev, bool enable);

#endif /* ZEPHYR_INCLUDE_INPUT_PMW3610_H_ */
