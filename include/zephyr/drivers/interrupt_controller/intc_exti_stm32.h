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
 * @brief Enable EXTI line.
 * @note Depending on mode either interrupt or event for corresponding line
 *       will be enabled. IRQ callback will be	set if provided.
 *
 * @param line_num	EXTI line number
 * @param trigger	EXTI trigger type (see @ref stm32_exti_trigger_type)
 * @param mode		EXTI mode (see @ref stm32_exti_mode)
 * @returns 0 on success, -EINVAL if @p line is invalid
 */
int stm32_exti_enable(uint32_t line_num, stm32_exti_trigger_type trigger,
					  stm32_exti_mode mode);

/**
 * @brief Disable EXTI line.
 *        This function is doing following steps in the exact order:
 *           - Set EXTI line mode to STM32_EXTI_MODE_NONE
 *           - Set EXTI line trigger ti STM32_EXTI_TRIG_NONE
 * @note Interrupt and event for given line number will be disabled and trigger
 *       will be set to none. Also interrupt callback will be removed.
 *
 * @param line_num	EXTI line number
 * @returns 0 on success, -EINVAL if @p line is invalid
 */
int stm32_exti_disable(uint32_t line_num);

/**
 * @brief Checks interrupt pending bit for specified EXTI line
 *
 * @param line_num EXTI line number
 * @returns true if @p line is pending, false otherwise
 */
bool stm32_exti_is_pending(uint32_t line_num);

/**
 * @brief Clears interrupt pending bit for specified EXTI line
 *
 * @param line_num	EXTI line number
 * @returns 0 - no pending signal on line, not 0 - pending signal on line
 */
int stm32_exti_clear_pending(uint32_t line_num);

/**
 * @brief Generates SW interrupt for specified EXTI line number
 *
 * @param line_num	EXTI line number
 * @returns 0 - no pending signal on line, not 0 - pending signal on line
 */
int stm32_exti_sw_interrupt(uint32_t line_num);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_EXTI_STM32_H_ */
