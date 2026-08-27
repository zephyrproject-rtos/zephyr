/*
 * Copyright (c) 2022 Esco Medical ApS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended LED Strip API of TLC5971 LED strip controller
 * @ingroup tlc5971_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_LED_STRIP_TLC5971_H_
#define ZEPHYR_INCLUDE_DRIVERS_LED_STRIP_TLC5971_H_

/**
 * @brief Texas Instruments TLC5971 (and compatible) LED strip controllers
 * @defgroup tlc5971_interface TLC5971
 * @ingroup led_strip_interface_ext
 * @{
 */

#include <zephyr/drivers/led_strip.h>

/**
 * @brief Maximum value for global brightness control, i.e 100% brightness
 */
#define TLC5971_GLOBAL_BRIGHTNESS_CONTROL_MAX 127

/**
 * @brief Set the global brightness control (BC) levels of a TLC5971 strip.
 *
 * The TLC5971 has a 7-bit global brightness control (BC) for each color group (Red, Green, Blue).
 * This function allows setting these values.
 *
 * The brightness value of each channel of @p pixel (r, g, b) maps to the corresponding global
 * brightness control register (BCR, BCG, BCB) and must be lower than or equal to
 * @ref TLC5971_GLOBAL_BRIGHTNESS_CONTROL_MAX.
 *
 * The change takes effect on the next update of the LED strip.
 *
 * @param dev LED strip device
 * @param pixel Global brightness values for RGB channels
 *
 * @return 0 on success, negative on error
 */
int tlc5971_set_global_brightness(const struct device *dev, struct led_rgb pixel);

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_LED_STRIP_TLC5971_H_ */
