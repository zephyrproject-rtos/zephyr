/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MISC_RENESAS_RA_EXTERNAL_INTERRUPT_H_
#define ZEPHYR_DRIVERS_MISC_RENESAS_RA_EXTERNAL_INTERRUPT_H_

#include <zephyr/drivers/gpio.h>

struct gpio_ra_callback {
	struct device *port;
	uint8_t port_num;
	uint8_t pin;
	enum gpio_int_trig trigger;
	enum gpio_int_mode mode;
	void (*isr)(const struct device *dev, gpio_pin_t pin);
};

int gpio_ra_interrupt_set(const struct device *dev, struct gpio_ra_callback *callback);
void gpio_ra_interrupt_unset(const struct device *dev, uint8_t port_num, uint8_t pin);

#endif /* ZEPHYR_DRIVERS_MISC_RENESAS_RA_EXTERNAL_INTERRUPT_H_ */
