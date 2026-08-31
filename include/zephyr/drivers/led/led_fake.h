/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fake LED controller driver API functions.
 * @ingroup led_fake
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_LED_LED_FAKE_H_
#define ZEPHYR_INCLUDE_DRIVERS_LED_LED_FAKE_H_

#include <zephyr/drivers/led.h>
#include <zephyr/fff.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fake LED controller driver
 * @defgroup led_fake Fake LED controller
 * @ingroup io_emulators
 * @ingroup led_interface
 *
 * The fake LED controller driver implements every LED API callback as a Fake
 * Function Framework (FFF) fake. It is enabled by @kconfig{CONFIG_LED_FAKE}
 * and instantiated from @dtcompatible{zephyr,fake-leds} devicetree nodes.
 *
 * Each fake is named after the API function it backs (`fake_led_on()` for
 * `led_on()`, and so on) and is paired with an FFF control structure carrying an
 * additional `_fake` suffix (`fake_led_on_fake`). Test suites include this
 * header to set return values, install custom fakes, or inspect call counts
 * and captured arguments. See @rstref{mocking-fff}.
 *
 * When @kconfig{CONFIG_ZTEST} is enabled, a ztest rule resets all fakes before
 * each test case. The reset also re-installs a default `custom_fake` for
 * `fake_led_get_info()` and `fake_led_set_color()`, which resolve the LED index
 * and colour count against the devicetree node. FFF gives `custom_fake`
 * precedence over `return_val`, so clear it before setting a return value for
 * those two.
 *
 * @code{.c}
 * const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(fake_leds));
 *
 * fake_led_on_fake.return_val = -EIO;
 *
 * zassert_equal(-EIO, led_on(dev, 2));
 * zassert_equal(1, fake_led_on_fake.call_count);
 * zassert_equal(2, fake_led_on_fake.arg1_val);
 * @endcode
 *
 * @{
 */

/** @cond INTERNAL_HIDDEN */

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

/** @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_LED_LED_FAKE_H_ */
