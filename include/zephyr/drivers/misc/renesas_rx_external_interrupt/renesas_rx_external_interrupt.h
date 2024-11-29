/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MISC_RENESAS_RX_EXTERNAL_INTERRUPT_H_
#define ZEPHYR_DRIVERS_MISC_RENESAS_RX_EXTERNAL_INTERRUPT_H_

#include <zephyr/drivers/gpio.h>

struct gpio_rx_callback {
	struct device *port;
	uint8_t port_num;
	uint8_t pin;
	enum gpio_int_trig trigger;
	enum gpio_int_mode mode;
	void (*isr)(const struct device *dev, gpio_pin_t pin);
};

int gpio_rx_interrupt_set(const struct device *dev, struct gpio_rx_callback *callback);
void gpio_rx_interrupt_unset(const struct device *dev, uint8_t port_num, uint8_t pin);

#endif /* ZEPHYR_DRIVERS_MISC_RENESAS_RX_EXTERNAL_INTERRUPT_H_ */
