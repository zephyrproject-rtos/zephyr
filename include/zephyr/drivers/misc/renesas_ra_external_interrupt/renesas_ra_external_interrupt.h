/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Interfaces for Renesas RA external interrupt.
 * @{
 */
#ifndef ZEPHYR_DRIVERS_MISC_RENESAS_RA_EXTERNAL_INTERRUPT_H_
#define ZEPHYR_DRIVERS_MISC_RENESAS_RA_EXTERNAL_INTERRUPT_H_

#include <zephyr/drivers/gpio.h>

/**
 * @brief Callback configuration for an external RA GPIO interrupt.
 *
 */
struct gpio_ra_callback {
	struct device *port;                                   /**< GPIO port device. */
	uint8_t port_num;                                      /**< Port index number. */
	uint8_t pin;                                           /**< Pin number. */
	enum gpio_int_trig trigger;                            /**< Trigger condition. */
	enum gpio_int_mode mode;                               /**< Interrupt mode configuration. */
	void (*isr)(const struct device *dev, gpio_pin_t pin); /**< ISR handler. */
};

/**
 * @brief Configure and enable RA external interrupt.
 *
 * @param dev       RA external interrupt device instance.
 * @param callback  Pointer to callback configuration.
 *
 * @return 0 on success, negative errno on failure.
 */
int gpio_ra_interrupt_set(const struct device *dev, struct gpio_ra_callback *callback);

/**
 * @brief Disable RA external interrupt.
 *
 * @param dev       RA external interrupt device instance.
 * @param port_num  Port index.
 * @param pin       Pin number.
 */
void gpio_ra_interrupt_unset(const struct device *dev, uint8_t port_num, uint8_t pin);

/** @} */

#endif /* ZEPHYR_DRIVERS_MISC_RENESAS_RA_EXTERNAL_INTERRUPT_H_ */
