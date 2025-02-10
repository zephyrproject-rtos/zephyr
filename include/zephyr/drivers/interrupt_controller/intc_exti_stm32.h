/*
 * Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief EXTI interrupt controller API for STM32 MCUs
 *
 * This API is used to interact with the EXTI interrupt controller
 * of STM32 microcontrollers.
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_EXTI_STM32_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_EXTI_STM32_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief EXTI interrupt trigger type
 */
enum stm32_exti_trigger_type {
	/* No trigger */
	STM32_EXTI_TRIG_NONE  = 0x0,
	/* Trigger on rising edge */
	STM32_EXTI_TRIG_RISING  = 0x1,
	/* Trigger on falling edge */
	STM32_EXTI_TRIG_FALLING = 0x2,
	/* Trigger on both rising and falling edge */
	STM32_EXTI_TRIG_BOTH = 0x3,
	/* Trigger on high level */
	STM32_EXTI_TRIG_HIGH_LEVEL = 0x4,
	/* Trigger on low level */
	STM32_EXTI_TRIG_LOW_LEVEL = 0x5
};

/**
 * @brief EXTI line mode
 */
enum stm32_exti_mode {
	/* Generate interrupts only */
	STM32_EXTI_MODE_IT  = 0x0,
	/* Generate events only */
	STM32_EXTI_MODE_EVENT  = 0x1,
	/* Generate interrupts and events */
	STM32_EXTI_MODE_BOTH = 0x2,
};

/**
 * @brief Set EXTI trigger type for a line and enables it.
 *
 * @param linenum EXTI line number
 * @param trigger EXTI trigger type (see @ref stm32_exti_trigger_type)
 */
void stm32_exti_set_trigger_type(uint32_t linenum, uint32_t trigger);

/**
 * @brief Set EXTI mode for a line
 *
 * @param linenum EXTI line number
 * @param mode EXTI mode (see @ref stm32_exti_mode)
 */
void stm32_exti_set_mode(uint32_t linenum, uint32_t mode);

/**
 * @brief Clears interrupt pending bit for specified EXTI line number
 *
 * @param linenum EXTI line number
 */
void stm32_exti_clear_pending(uint32_t linenum);

/**
 * @brief Checks interrupt pending bit for specified EXTI line number
 *
 * @param linenum EXTI line number
 */
uint32_t stm32_exti_is_pending(uint32_t linenum);

/**
 * @brief Set which GPIO port triggers events on specified EXTI line number
 *
 * @param linenum	EXTI line number
 * @param port	GPIO port number (STM32_PORTA, STM32_PORTB, ...)
 */
void stm32_exti_set_line_src_port(uint32_t linenum, uint32_t port);

/**
 * @brief Get port which is triggering events on specified EXTI line number
 *
 * @param linenum	EXTI line number
 * @returns GPIO port number (STM32_PORTA, STM32_PORTB, ...)
 */
uint32_t stm32_exti_get_line_src_port(uint32_t linenum);

/**
 * @brief Enable EXTI interrupts for specified line number
 * @note TODO: Shall be dropped since trigger mode enables either IT or Event
 *
 * @param linenum	EXTI line number
 */
void stm32_exti_enable_irq(uint32_t linenum);

/**
 * @brief Disable EXTI interrupts for specified line number
 * @note TODO: Shall be dropped since trigger mode enables either IT or Event
 *
 * @param linenum	EXTI line number
 */
void stm32_exti_disable_irq(uint32_t linenum);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_EXTI_STM32_H_ */
