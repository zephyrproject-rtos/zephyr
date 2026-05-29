/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Interfaces for Renesas RX external interrupt.
 * @{
 */

#ifndef ZEPHYR_DRIVERS_MISC_RENESAS_RX_EXTERNAL_INTERRUPT_H_
#define ZEPHYR_DRIVERS_MISC_RENESAS_RX_EXTERNAL_INTERRUPT_H_

#include <zephyr/drivers/gpio.h>

/**
 * @brief Callback configuration for external RX GPIO interrupt.
 *
 */
struct gpio_rx_callback {
	struct device *port;                                   /**< GPIO port device. */
	uint8_t port_num;                                      /**< Port index number. */
	uint8_t pin;                                           /**< Pin number. */
	enum gpio_int_trig trigger;                            /**< Trigger condition. */
	enum gpio_int_mode mode;                               /**< Interrupt mode configuration. */
	void (*isr)(const struct device *dev, gpio_pin_t pin); /**< ISR handler. */
};

/**
 * @brief Configure and enable an RX external interrupt.
 *
 * @param dev       RX external interrupt device instance.
 * @param callback  Pointer to callback configuration.
 *
 * @return 0 on success, negative errno on failure.
 */
int gpio_rx_interrupt_set(const struct device *dev, struct gpio_rx_callback *callback);

/**
 * @brief Disable RX external interrupt.
 *
 * @param dev       RX external interrupt device instance.
 * @param port_num  Port index.
 * @param pin       Pin number.
 */
void gpio_rx_interrupt_unset(const struct device *dev, uint8_t port_num, uint8_t pin);

/** @} */

#endif /* ZEPHYR_DRIVERS_MISC_RENESAS_RX_EXTERNAL_INTERRUPT_H_ */
