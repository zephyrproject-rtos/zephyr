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
typedef enum {
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
} stm32_exti_trigger_type;

/**
 * @brief EXTI line mode
 */
typedef enum {
	/* Generate interrupts only */
	STM32_EXTI_MODE_IT  = 0x0,
	/* Generate events only */
	STM32_EXTI_MODE_EVENT  = 0x1,
	/* Generate interrupts and events */
	STM32_EXTI_MODE_BOTH = 0x2,
	/* Disable interrupts and events */
	STM32_EXTI_MODE_NONE = 0x3,
} stm32_exti_mode;

/**
 * @brief EXTI interrupt callback function signature
 *
 * @param linenum	EXTI line number on which interrupt occurred
 * @param user	@p data provided to @ref stm32_exti_enable
 *
 * @note This callback is invoked in ISR context.
 */
typedef void (*stm32_exti_irq_cb_t)(uint32_t linenum, void *user);

/**
 * @brief Enable EXTI line given by line number
 * @note Depending on mode either interrupt or event for corresponding line
 *       will be enabled. IRQ callback will bes	set if provided.
 *
 * @param linenum	EXTI line number
 * @param trigger	EXTI trigger type (see @ref stm32_exti_trigger_type)
 * @param mode		EXTI mode (see @ref stm32_exti_mode)
 * @param cb		Interrupt callback function
 * @param user	Custom user data for usage by the callback
 * @returns 0 on success, -EBUSY if a callback is already set for @p linenum,
 *                        -EINVAL if @p linenum is invalid
 */
int stm32_exti_enable(uint32_t linenum, stm32_exti_trigger_type trigger,
					 stm32_exti_mode mode, stm32_exti_irq_cb_t cb, void *user);

/**
 * @brief Disable EXTI line given by line number.
 *        This function is doing following steps in the exact order:
 *           - Set EXTI line mode to STM32_EXTI_MODE_NONE
 *           - Set EXTI line trigger ti STM32_EXTI_TRIG_NONE
 *           - Remove EXTI line callback
 * @note Interrupt and event for given line number will be disabled and trigger
 *       will be set to none. Also interrupt callback will be removed.
 *
 * @param linenum	EXTI line number
 * @returns 0 on success, -EINVAL if @p linenum is invalid
 */
int stm32_exti_disable(uint32_t linenum);

/**
 * @brief Set which GPIO port triggers events on specified EXTI line number
 *
 * @param linenum	EXTI line number
 * @param port	GPIO port number (STM32_PORTA, STM32_PORTB, ...)
 * @returns 0 on success, -EINVAL if @p linenum is invalid
 */
int stm32_exti_set_line_src_port(uint32_t linenum, uint32_t port);

/**
 * @brief Get port which is triggering events on specified EXTI line number
 *
 * @param linenum	EXTI line number
 * @param port		Pointer to store GPIO port number (STM32_PORTA, STM32_PORTB, ...)
 * @returns 0 on success, -EINVAL if @p linenum is invalid
 */
int stm32_exti_get_line_src_port(uint32_t linenum, uint32_t *port);

/**
 * @brief Enable EXTI interrupts for specified line number
 *
 * @note This function will assert with no message if the line number is invalid
 *
 * @param linenum	EXTI line number
 */
void stm32_exti_enable_irq(uint32_t linenum);

/**
 * @brief Disable EXTI interrupts for specified line number
 *
 * @note This function will assert with no message if the line number is invalid
 *
 * @param linenum	EXTI line number
 */
void stm32_exti_disable_irq(uint32_t linenum);

/**
 * @brief Enable EXTI event for specified line number
 *
 * @param linenum	EXTI line number
 * @returns 0 on success, -EINVAL if @p linenum is invalid
 */
int stm32_exti_enable_event(uint32_t linenum);

/**
 * @brief Disable EXTI interrupts for specified line number
 *
 * @param linenum	EXTI line number
 * @returns 0 on success, -EINVAL if @p linenum is invalid
 */
int stm32_exti_disable_event(uint32_t linenum);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_EXTI_STM32_H_ */
