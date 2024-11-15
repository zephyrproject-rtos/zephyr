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

/**
 * @brief Opaque type representing a GPIO interrupt line
 */
typedef uint32_t stm32_gpio_irq_line_t;

/**
 * @brief Get the GPIO interrupt line value corresponding
 *        to specified @p pin of GPIO port @p port
 */
stm32_gpio_irq_line_t stm32_gpio_intc_get_pin_irq_line(uint32_t port, gpio_pin_t pin);

/**
 * @brief Enable GPIO interrupts for specified line
 *
 * @param line	GPIO interrupt line
 */
void stm32_gpio_intc_enable_line(stm32_gpio_irq_line_t line);

/**
 * @brief Disable GPIO interrupts for specified line
 *
 * @param line	GPIO interrupt line
 */
void stm32_gpio_intc_disable_line(stm32_gpio_irq_line_t line);

/**
 * @brief GPIO interrupt trigger flags
 */
enum stm32_gpio_irq_trigger {
	/* No trigger */
	STM32_GPIO_IRQ_TRIG_NONE  = 0x0,
	/* Trigger on rising edge */
	STM32_GPIO_IRQ_TRIG_RISING  = 0x1,
	/* Trigger on falling edge */
	STM32_GPIO_IRQ_TRIG_FALLING = 0x2,
	/* Trigger on both rising and falling edge */
	STM32_GPIO_IRQ_TRIG_BOTH = 0x3,
	/* Trigger on high level */
	STM32_GPIO_IRQ_TRIG_HIGH_LEVEL = 0x4,
	/* Trigger on low level */
	STM32_GPIO_IRQ_TRIG_LOW_LEVEL = 0x5
};

/**
 * @brief Select trigger for interrupt on specified GPIO line
 *
 * @param line	GPIO interrupt line
 * @param trg	Interrupt trigger (see @ref stm32_gpio_irq_trigger)
 */
void stm32_gpio_intc_select_line_trigger(stm32_gpio_irq_line_t line, uint32_t trg);

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
int stm32_gpio_intc_set_irq_callback(stm32_gpio_irq_line_t line,
					stm32_gpio_irq_cb_t cb, void *user);

/**
 * @brief Removes the interrupt callback of specified EXTI line
 *
 * @param line	EXTI interrupt line
 */
void stm32_gpio_intc_remove_irq_callback(stm32_gpio_irq_line_t line);

/** Hardware-specific API extensions */

#if defined(CONFIG_EXTI_STM32)	/* EXTI-specific extensions */
/**
 * @brief Set which GPIO port triggers events on specified EXTI line.
 *
 * @param line	EXTI line number (= pin number)
 * @param port	GPIO port number (STM32_PORTA, STM32_PORTB, ...)
 */
void stm32_exti_set_line_src_port(gpio_pin_t line, uint32_t port);

/**
 * @brief Get port which is triggering events on specified EXTI line.
 *
 * @param line	EXTI line number (= pin number)
 * @returns GPIO port number (STM32_PORTA, STM32_PORTB, ...)
 */
uint32_t stm32_exti_get_line_src_port(gpio_pin_t line);
#endif /* CONFIG_EXTI_STM32 */

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_GPIO_INTC_STM32_H_ */
