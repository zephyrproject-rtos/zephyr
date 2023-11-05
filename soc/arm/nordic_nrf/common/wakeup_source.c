/*
 * Copyright (c) 2023 Bjarki Arge Andreasen
 * SPDX-License-Identifier: Apache-2.0
 */

#warning "DISCLAIMER Very naive implementation for demonstration purposes only"

#include <zephyr/kernel.h>
#include <zephyr/sys/wakeup_source.h>
#include <hal/nrf_gpio.h>

struct gpio_nrfx_cfg {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	NRF_GPIO_Type *port;
	uint32_t edge_sense;
	uint8_t port_num;
};

void z_sys_wakeup_source_enable_gpio(const struct gpio_dt_spec *gpio)
{
	const struct gpio_nrfx_cfg *cfg = gpio->port->config;
	uint32_t abs_pin = NRF_GPIO_PIN_MAP(cfg->port_num, gpio->pin);
	nrf_gpio_pin_pull_t pull;
	nrf_gpio_pin_sense_t sense;

	if (gpio->dt_flags & GPIO_ACTIVE_LOW) {
		sense = NRF_GPIO_PIN_SENSE_LOW;
	} else {
		sense = NRF_GPIO_PIN_SENSE_HIGH;
	}

	if (gpio->dt_flags & GPIO_PULL_UP) {
		pull = NRF_GPIO_PIN_PULLUP;
	} else if (gpio->dt_flags & GPIO_PULL_DOWN) {
		pull = NRF_GPIO_PIN_PULLDOWN;
	} else {
		pull = NRF_GPIO_PIN_NOPULL;
	}

	nrf_gpio_cfg(abs_pin,
		     NRF_GPIO_PIN_DIR_INPUT,
		     NRF_GPIO_PIN_INPUT_CONNECT,
		     pull,
		     NRF_GPIO_PIN_S0S1,
		     sense);
}
