/*
 * Copyright (c) 2025 Gielen De Maesschalck <gielen.de.maesschalck@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INCLUDE_DRIVERS_LED_STRIP_LED_STRIP_FAKE_H_
#define INCLUDE_DRIVERS_LED_STRIP_LED_STRIP_FAKE_H_

#include <zephyr/drivers/led_strip.h>
#include <zephyr/fff.h>

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_FAKE_VALUE_FUNC(int, fake_led_strip_update_rgb, const struct device *, struct led_rgb *, size_t);
DECLARE_FAKE_VALUE_FUNC(size_t, fake_led_strip_length, const struct device *);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_DRIVERS_LED_STRIP_LED_STRIP_FAKE_H_ */