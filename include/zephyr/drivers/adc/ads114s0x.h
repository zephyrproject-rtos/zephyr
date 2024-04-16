/*
 * Copyright (c) 2023 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ADC_ADS114S0X_H_
#define ZEPHYR_INCLUDE_DRIVERS_ADC_ADS114S0X_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

int ads114s0x_gpio_set_output(const struct device *dev, uint8_t pin, bool initial_value);

int ads114s0x_gpio_set_input(const struct device *dev, uint8_t pin);

int ads114s0x_gpio_deconfigure(const struct device *dev, uint8_t pin);

int ads114s0x_gpio_set_pin_value(const struct device *dev, uint8_t pin,
				bool value);

int ads114s0x_gpio_get_pin_value(const struct device *dev, uint8_t pin,
				bool *value);

int ads114s0x_gpio_port_get_raw(const struct device *dev,
			       gpio_port_value_t *value);

int ads114s0x_gpio_port_set_masked_raw(const struct device *dev,
				      gpio_port_pins_t mask,
				      gpio_port_value_t value);

int ads114s0x_gpio_port_toggle_bits(const struct device *dev,
				   gpio_port_pins_t pins);

#endif /* ZEPHYR_INCLUDE_DRIVERS_ADC_ADS114S0X_H_ */
