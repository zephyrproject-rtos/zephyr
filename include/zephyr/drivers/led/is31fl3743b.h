/*
 * Copyright (c) 2026 Open Device Partnership and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_LED_IS31FL3743B_H_
#define ZEPHYR_INCLUDE_DRIVERS_LED_IS31FL3743B_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>

/**
 * @brief Blanks IS31FL3743B LED display.
 *
 * When blank_en is set, the LED display will be disabled. This can be used for
 * flicker-free display updates or power saving.
 *
 * @param dev LED device structure
 * @param blank_en should blanking be enabled
 * @return 0 on success or negative value on error.
 */
int is31fl3743b_blank(const struct device *dev, bool blank_en);

/**
 * @brief Sets LED current limit.
 *
 * Sets the global current limit for the LED driver. This is a separate value
 * from per-LED brightness and applies to all LEDs. See the Global Current Control
 * Register docs in and below Table 7 on page 14 of the datasheet.
 *
 * This value sets the output current limit according to
 * the following formula: (343/R_ISET) * (limit/256).
 * This formula corresponds to Formula (3) on page 14 of the datasheet.
 *
 * @param dev LED device structure
 * @param limit current limit to apply
 * @return 0 on success, or negative value on error.
 */
int is31fl3743b_current_limit(const struct device *dev, uint8_t limit);

#endif /* ZEPHYR_INCLUDE_DRIVERS_LED_IS31FL3743B_H_ */
