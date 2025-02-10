/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief GPIO interrupt controller API for STM32 MCUs
 *
 * This API is used to interact with the GPIO interrupt controller
 * of STM32 microcontrollers.
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_GPIO_INTC_STM32_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_GPIO_INTC_STM32_H_

#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/interrupt_controller/intc_exti_stm32.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the GPIO interrupt line value corresponding
 *        to specified @p pin of GPIO port @p port
 */
uint32_t stm32_gpio_intc_get_pin_irq_line(uint32_t port, gpio_pin_t pin);

/**
 * @brief GPIO interrupt callback function signature
 *
 * @param pin	GPIO pin on which interrupt occurred
 * @param user	@p data provided to @ref stm32_gpio_intc_set_irq_callback
 *
 * @note This callback is invoked in ISR context.
 */
typedef void (*stm32_gpio_irq_cb_t)(gpio_port_pins_t pin, void *user);

/**
 * @brief Set callback invoked when an interrupt occurs on specified GPIO line
 *
 * @param line	GPIO interrupt line
 * @param cb	Interrupt callback function
 * @param user	Custom user data for usage by the callback
 * @returns 0 on success, -EBUSY if a callback is already set for @p line
 */
int stm32_gpio_intc_set_irq_callback(uint32_t line,
					stm32_gpio_irq_cb_t cb, void *user);

/**
 * @brief Removes the interrupt callback of specified EXTI line
 *
 * @param line	EXTI interrupt line
 */
void stm32_gpio_intc_remove_irq_callback(uint32_t line);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_GPIO_INTC_STM32_H_ */
