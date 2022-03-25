/*
 * Copyright (c) 2022 Esco Medical ApS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_LED_STRIP_TLC5971_H_
#define ZEPHYR_INCLUDE_DRIVERS_LED_STRIP_TLC5971_H_

/**
 * @brief Maximum value for global brightness control, i.e 100% brightness
 */
#define TLC5971_GLOBAL_BRIGHTNESS_CONTROL_MAX 127

/**
 * @brief Set the global brightness control levels for the tlc5971 strip.
 *
 * change will take effect on next update of the led strip
 *
 * @param dev LED strip device
 * @param pixel global brightness values for RGB channels

 * @return 0 on success, negative on error
 */
int tlc5971_set_global_brightness(const struct device *dev, struct led_rgb pixel);

#endif /* ZEPHYR_INCLUDE_DRIVERS_LED_STRIP_TLC5971_H_ */
