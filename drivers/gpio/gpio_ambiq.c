/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>

#include <am_mcu_apollo.h>

#include "gpio_ambiq.h"

int ambiq_gpio_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					  gpio_port_value_t value)
{
	const struct ambiq_gpio_config *const dev_cfg = dev->config;

	for (int i = 0; i < dev_cfg->ngpios; i++) {
		if ((mask >> i) & 1) {
			am_hal_gpio_state_write(i + dev_cfg->pin_offset, ((value >> i) & 1));
		}
	}

	return 0;
}

int ambiq_gpio_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct ambiq_gpio_config *const dev_cfg = dev->config;

	for (int i = 0; i < dev_cfg->ngpios; i++) {
		if ((pins >> i) & 1) {
			am_hal_gpio_state_write(i + dev_cfg->pin_offset, AM_HAL_GPIO_OUTPUT_SET);
		}
	}

	return 0;
}

int ambiq_gpio_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct ambiq_gpio_config *const dev_cfg = dev->config;

	for (int i = 0; i < dev_cfg->ngpios; i++) {
		if ((pins >> i) & 1) {
			am_hal_gpio_state_write(i + dev_cfg->pin_offset, AM_HAL_GPIO_OUTPUT_CLEAR);
		}
	}

	return 0;
}

int ambiq_gpio_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct ambiq_gpio_config *const dev_cfg = dev->config;

	for (int i = 0; i < dev_cfg->ngpios; i++) {
		if ((pins >> i) & 1) {
			am_hal_gpio_state_write(i + dev_cfg->pin_offset, AM_HAL_GPIO_OUTPUT_TOGGLE);
		}
	}

	return 0;
}

int ambiq_gpio_manage_callback(const struct device *dev, struct gpio_callback *callback,
				      bool set)
{
	struct ambiq_gpio_data *const data = dev->data;

	return gpio_manage_callback(&data->cb, callback, set);
}

