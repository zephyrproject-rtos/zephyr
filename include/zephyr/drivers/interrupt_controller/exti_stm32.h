/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for External interrupt/event controller in STM32 MCUs
 *
 * Based on reference manuals:
 *   RM0008 Reference Manual: STM32F101xx, STM32F102xx, STM32F103xx, STM32F105xx
 *   and STM32F107xx advanced ARM(r)-based 32-bit MCUs
 * and
 *   RM0368 Reference manual STM32F401xB/C and STM32F401xD/E
 *   advanced ARM(r)-based 32-bit MCUs
 *
 * Chapter 10.2: External interrupt/event controller (EXTI)
 *
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_EXTI_STM32_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_EXTI_STM32_H_

#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>

/**
 * @brief Type representing an EXTI line number
 */
typedef uint32_t stm32_exti_line_t;

/**
 * @brief Enable EXTI interrupts for specified line
 *
 * @param line	EXTI interrupt line
 */
void stm32_exti_enable(stm32_exti_line_t line);

/**
 * @brief Disable EXTI interrupts for specified line
 *
 * @param line	EXTI interrupt line
 */
void stm32_exti_disable(stm32_exti_line_t line);

/**
 * @brief EXTI interrupt trigger flags
 */
enum stm32_exti_trigger {
	/* No trigger */
	STM32_EXTI_TRIG_NONE  = 0x0,
	/* Trigger on rising edge */
	STM32_EXTI_TRIG_RISING  = 0x1,
	/* Trigger on falling edge */
	STM32_EXTI_TRIG_FALLING = 0x2,
	/* Trigger on both rising and falling edge */
	STM32_EXTI_TRIG_BOTH = 0x3,
};

/**
 * @brief Select trigger for interrupt on specified EXTI line
 *
 * @param line	EXTI interrupt line
 * @param trg	Interrupt trigger (see @ref stm32_exti_trigger)
 */
void stm32_exti_trigger(stm32_exti_line_t line, uint32_t trg);

/**
 * @brief EXTI interrupt callback function signature
 *
 * @param line	Triggered EXTI interrupt line
 * @param user	@p data provided to @ref stm32_exti_set_callback
 *
 * @note This callback is invoked in ISR context.
 */
typedef void (*stm32_exti_callback_t)(stm32_exti_line_t line, void *user);

/**
 * @brief Set callback invoked when an interrupt occurs on specified EXTI line
 *
 * @param line	EXTI interrupt line
 * @param cb	Interrupt callback function
 * @param data	Custom data for usage by the callback
 * @returns 0 on success, -EBUSY if a callback is already set for @p line
 */
int stm32_exti_set_callback(stm32_exti_line_t line, stm32_exti_callback_t cb, void *data);

/**
 * @brief Removes the interrupt callback of specified EXTI line
 *
 * @param line	EXTI interrupt line
 */
void stm32_exti_unset_callback(stm32_exti_line_t line);

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

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_EXTI_STM32_H_ */
