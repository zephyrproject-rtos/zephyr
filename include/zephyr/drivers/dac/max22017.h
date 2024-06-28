/*
 * Copyright (c) 2024 Analog Devices Inc.
 * Copyright (c) 2024 Baylibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DAC_MAX22017__H_
#define ZEPHYR_INCLUDE_DRIVERS_DAC_MAX22017__H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

int adi_max22017_gpio_set_output(const struct device *dev, uint8_t pin, bool initial_value);

int adi_max22017_gpio_set_input(const struct device *dev, uint8_t pin);

int adi_max22017_gpio_deconfigure(const struct device *dev, uint8_t pin);

int adi_max22017_gpio_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					      enum gpio_int_mode mode, enum gpio_int_trig trig);

int adi_max22017_gpio_set_pin_value(const struct device *dev, uint8_t pin, bool value);

int adi_max22017_gpio_get_pin_value(const struct device *dev, uint8_t pin, bool *value);

int adi_max22017_gpio_port_get_raw(const struct device *dev, gpio_port_value_t *value);

int adi_max22017_gpio_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					  gpio_port_value_t value);

int adi_max22017_gpio_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins);

int adi_max22017_manage_cb(const struct device *dev, struct gpio_callback *callback, bool set);

#endif /* ZEPHYR_INCLUDE_DRIVERS_DAC_MAX22017__H_ */
