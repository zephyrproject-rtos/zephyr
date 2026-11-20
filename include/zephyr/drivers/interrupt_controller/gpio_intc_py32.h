/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Internal GPIO interrupt controller interface for Puya PY32 devices.
 * @ingroup gpio_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_GPIO_INTC_PY32_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_GPIO_INTC_PY32_H_

/** @cond INTERNAL_HIDDEN */

#include <zephyr/drivers/gpio.h>

/** @brief Callback for one PY32 GPIO interrupt line. */
typedef void (*py32_exti_callback_t)(gpio_port_pins_t pins, void *user);

/**
 * @brief Configure a GPIO interrupt line and its callback.
 * @param port GPIO port index (A = 0, B = 1, F = 5).
 * @param pin GPIO pin number.
 * @param mode Interrupt mode.
 * @param trig Interrupt trigger.
 * @param cb Callback invoked in interrupt context.
 * @param user Callback context.
 * @retval 0 Configuration applied.
 * @retval -EINVAL Invalid pin or port.
 * @retval -ENOTSUP Unsupported interrupt mode or source.
 * @retval -EBUSY Line owned by another port.
 */
int py32_exti_configure(uint32_t port, gpio_pin_t pin, enum gpio_int_mode mode,
			enum gpio_int_trig trig, py32_exti_callback_t cb, void *user);

/**
 * @brief Set the GPIO source of a PY32 EXTI line.
 * @param line EXTI line number, from 0 through 15.
 * @param port GPIO port index (A = 0, B = 1, F = 5).
 */
void py32_exti_set_line_src_port(gpio_pin_t line, uint32_t port);

/**
 * @brief Read the GPIO source of a PY32 EXTI line.
 * @param line EXTI line number, from 0 through 15.
 * @return GPIO port index (A = 0, B = 1, F = 5).
 */
uint32_t py32_exti_get_line_src_port(gpio_pin_t line);

/** @endcond */

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_GPIO_INTC_PY32_H_ */
