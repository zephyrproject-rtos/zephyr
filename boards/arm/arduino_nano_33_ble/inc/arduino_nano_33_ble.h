/*
 * Copyright (c) 2020 Jefferson Lee.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include "arduino_nano_33_ble_pins.h"

/* this header-only library makes GPIO access a little more arduino-y. */
#include <device.h>
#include <drivers/gpio.h>
struct arduino_gpio_t {
	const struct device *gpios[2];
};

static inline void arduino_gpio_init(struct arduino_gpio_t *gpios)
{
	gpios->gpios[0] = device_get_binding("GPIO_0");
	gpios->gpios[1] = device_get_binding("GPIO_1");
}
static inline int arduino_gpio_pinMode(struct arduino_gpio_t *gpios,
						int pin, int mode)
{
	return gpio_pin_configure(gpios->gpios[pin / 32], pin % 32, mode);
}
static inline int arduino_gpio_digitalWrite(struct arduino_gpio_t *gpios,
						int pin, int value)
{
	return gpio_pin_set(gpios->gpios[pin / 32], pin % 32, value);
}
static inline int arduino_gpio_digitalRead(struct arduino_gpio_t *gpios,
						int pin)
{
	return gpio_pin_get(gpios->gpios[pin / 32], pin % 32);
}
