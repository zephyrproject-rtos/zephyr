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
 * @driver_fake{led_interface,CONFIG_LED_FAKE,zephyr\,fake-leds}
 *
 * `fake_led_get_info()` and `fake_led_set_color()` are given a default
 * `custom_fake` resolving the LED index and colour count against the devicetree
 * node, re-installed on every reset. Install your own `custom_fake` in the test
 * case to change what they report.
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

/** @fake_of{led_driver_api::on} */
DECLARE_FAKE_VALUE_FUNC(int, fake_led_on, const struct device *, uint32_t);

/** @fake_of{led_driver_api::off} */
DECLARE_FAKE_VALUE_FUNC(int, fake_led_off, const struct device *, uint32_t);

/** @fake_of{led_driver_api::set_brightness} */
DECLARE_FAKE_VALUE_FUNC(int, fake_led_set_brightness, const struct device *, uint32_t, uint8_t);

/** @fake_of{led_driver_api::blink} */
DECLARE_FAKE_VALUE_FUNC(int, fake_led_blink, const struct device *, uint32_t, uint32_t, uint32_t);

/** @fake_of{led_driver_api::get_info} */
DECLARE_FAKE_VALUE_FUNC(int, fake_led_get_info, const struct device *, uint32_t,
			const struct led_info **);

/** @fake_of{led_driver_api::set_color} */
DECLARE_FAKE_VALUE_FUNC(int, fake_led_set_color, const struct device *, uint32_t, uint8_t,
			const uint8_t *);

/** @fake_of{led_driver_api::write_channels} */
DECLARE_FAKE_VALUE_FUNC(int, fake_led_write_channels, const struct device *, uint32_t, uint32_t,
			const uint8_t *);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_LED_LED_FAKE_H_ */
