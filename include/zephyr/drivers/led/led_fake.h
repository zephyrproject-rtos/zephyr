/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fake LED controller driver API functions.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_LED_LED_FAKE_H_
#define ZEPHYR_INCLUDE_DRIVERS_LED_LED_FAKE_H_

#include <zephyr/drivers/led.h>
#include <zephyr/fff.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fake LED controller driver API functions.
 * @defgroup led_fake Fake LED controller
 * @ingroup io_emulators
 * @ingroup led_interface
 * @{
 */

/**
 * @brief Turn a fake LED controller LED on.
 *
 * @see led_on
 */
DECLARE_FAKE_VALUE_FUNC(int, fake_led_on, const struct device *, uint32_t);

/**
 * @brief Turn a fake LED controller LED off.
 *
 * @see led_off
 */
DECLARE_FAKE_VALUE_FUNC(int, fake_led_off, const struct device *, uint32_t);

/**
 * @brief Set brightness of a fake LED controller LED.
 *
 * @see led_set_brightness
 */
DECLARE_FAKE_VALUE_FUNC(int, fake_led_set_brightness, const struct device *, uint32_t, uint8_t);

/**
 * @brief Blink a fake LED controller LED.
 *
 * @see led_blink
 */
DECLARE_FAKE_VALUE_FUNC(int, fake_led_blink, const struct device *, uint32_t, uint32_t, uint32_t);

/**
 * @brief Get info of a fake LED controller LED.
 *
 * @see led_get_info
 */
DECLARE_FAKE_VALUE_FUNC(int, fake_led_get_info, const struct device *, uint32_t,
			const struct led_info **);

/**
 * @brief Set the color of a fake LED controller LED.
 *
 * @see led_set_color
 */
DECLARE_FAKE_VALUE_FUNC(int, fake_led_set_color, const struct device *, uint32_t, uint8_t,
			const uint8_t *);

/**
 * @brief Write/update a strip of a fake LED controller LEDs.
 *
 * @see led_write_channels
 */
DECLARE_FAKE_VALUE_FUNC(int, fake_led_write_channels, const struct device *, uint32_t, uint32_t,
			const uint8_t *);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_LED_LED_FAKE_H_ */
