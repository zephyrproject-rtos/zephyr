/*
 * Copyright 2023 Daniel DeGrasse <daniel@degrasse.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_LED_IS31FL3733_H_
#define ZEPHYR_INCLUDE_DRIVERS_LED_IS31FL3733_H_

/**
 * @brief Blanks IS31FL3733 LED display.
 *
 * When blank_en is set, the LED display will be disabled. This can be used for
 * flicker-free display updates, or power saving.
 *
 * @param dev: LED device structure
 * @param blank_en: should blanking be enabled
 * @return 0 on success, or negative value on error.
 */
int is31fl3733_blank(const struct device *dev, bool blank_en);

/**
 * @brief Sets led current limit
 *
 * Sets the current limit for the LED driver. This is a separate value
 * from per-led brightness, and applies to all LEDs.
 * This value sets the output current limit according
 * to the following formula: (840/R_ISET) * (limit/256)
 * See table 14 of the datasheet for additional details.
 * @param dev: LED device structure
 * @param limit: current limit to apply
 * @return 0 on success, or negative value on error.
 */
int is31fl3733_current_limit(const struct device *dev, uint8_t limit);

#endif /* ZEPHYR_INCLUDE_DRIVERS_LED_IS31FL3733_H_ */
