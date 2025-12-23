/*
 * Copyright 2024 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for PAW32xx input driver.
 * @ingroup paw32xx_interface
 */

#ifndef ZEPHYR_INCLUDE_INPUT_PAW32XX_H_
#define ZEPHYR_INCLUDE_INPUT_PAW32XX_H_

/**
 * @defgroup paw32xx_interface PAW32xx
 * @ingroup input_interface_ext
 * @brief PAW32xx ultra low power wireless mouse chip
 * @{
 */

/**
 * @brief Set resolution on a paw32xx device
 *
 * @param dev paw32xx device.
 * @param res_cpi CPI resolution, 200 to 3200.
 */
int paw32xx_set_resolution(const struct device *dev, uint16_t res_cpi);

/**
 * @brief Set force awake mode on a paw32xx device
 *
 * @param dev paw32xx device.
 * @param enable whether to enable or disable force awake mode.
 */
int paw32xx_force_awake(const struct device *dev, bool enable);

/** @} */

#endif /* ZEPHYR_INCLUDE_INPUT_PAW32XX_H_ */
