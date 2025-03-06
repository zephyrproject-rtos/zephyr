/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2024 Majtx
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief  interrupt controller API for AT32 MCUs
 *
 * This API is used to interact with the interrupt controller
 * of AT32 microcontrollers.
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_AT32_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_AT32_H_

#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>

/**
 * @brief Opaque type representing a GPIO interrupt line
 */
typedef uint32_t at32_irq_line_t;

/**
 * @brief Get the GPIO interrupt line value corresponding
 *        to specified @p pin of GPIO port @p port
 */
at32_irq_line_t at32_exint_intc_get_pin_irq_line(uint32_t port, gpio_pin_t pin);

/**
 * @brief Enable GPIO interrupts for specified line
 *
 * @param line	GPIO interrupt line
 */
void at32_exint_intc_enable_line(at32_irq_line_t line);

/**
 * @brief Disable GPIO interrupts for specified line
 *
 * @param line	GPIO interrupt line
 */
void at32_exint_intc_disable_line(at32_irq_line_t line);

/**
 * @brief GPIO interrupt trigger flags
 */
enum at32_exint_irq_trigger {
	/* No trigger */
	AT32_GPIO_IRQ_TRIG_NONE  = 0x0,
	/* Trigger on rising edge */
	AT32_GPIO_IRQ_TRIG_RISING  = 0x1,
	/* Trigger on falling edge */
	AT32_GPIO_IRQ_TRIG_FALLING = 0x2,
	/* Trigger on both rising and falling edge */
	AT32_GPIO_IRQ_TRIG_BOTH = 0x3,
	/* Trigger on high level */
	AT32_GPIO_IRQ_TRIG_HIGH_LEVEL = 0x4,
	/* Trigger on low level */
	AT32_GPIO_IRQ_TRIG_LOW_LEVEL = 0x5
};

/**
 * @brief Select trigger for interrupt on specified GPIO line
 *
 * @param line	GPIO interrupt line
 * @param trg	Interrupt trigger
 */
void at32_exint_intc_select_line_trigger(at32_irq_line_t line, uint32_t trg);

/**
 * @brief GPIO interrupt callback function signature
 *
 * @param pin	GPIO pin on which interrupt occurred
 * @param user	@p data provided to @ref at32_exint_intc_set_irq_callback
 *
 * @note This callback is invoked in ISR context.
 */
typedef void (*at32_exint_irq_cb_t)(uint32_t pin, void *user);

/**
 * @brief Set callback invoked when an interrupt occurs on specified GPIO line
 *
 * @param line interrupt line
 * @param cb	Interrupt callback function
 * @param data	Custom data for usage by the callback
 * @returns 0 on success, -EBUSY if a callback is already set for @p line
 */
int at32_exint_intc_set_irq_callback(at32_irq_line_t line,
					at32_exint_irq_cb_t cb, void *data);

/**
 * @brief Removes the interrupt callback of specified EXINT line
 *
 * @param line	EXINT interrupt line
 */
void at32_exint_intc_remove_irq_callback(at32_irq_line_t line);

/** Hardware-specific API extensions */


/**
 * @brief Set which GPIO port triggers events on specified EXTI line.
 *
 * @param line	EXINT line number (= pin number)
 * @param port	GPIO port number
 */
void at32_exint_set_line_src_port(gpio_pin_t line, uint32_t port);

/**
 * @brief Get port which is triggering events on specified EXINT line.
 *
 * @param line	EXINT line number (= pin number)
 * @returns GPIO port number
 */
uint32_t at32_exint_get_line_src_port(gpio_pin_t pin);

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_AT32_H_ */
