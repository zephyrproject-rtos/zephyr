/*
 * Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief STM32 EXTI interrupt controller API
 *
 * This API is used to interact with STM32 EXTI interrupt controller
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
 * @note The caller driver is responsible for registering an ISR and
 *        enabling the corresponding interrupt if MODE_IT is selected.
 *
 * @param line_num	EXTI line number
 * @param trigger	EXTI trigger type (see @ref stm32_exti_trigger_type)
 * @param mode		EXTI mode (see @ref stm32_exti_mode)
 * @returns 0 on success, -EINVAL if @p line_num is invalid
 */
int stm32_exti_enable(uint32_t line_num, stm32_exti_trigger_type trigger,
					  stm32_exti_mode mode);

/**
 * @brief Disable EXTI line.
 * After this function has been called, EXTI line @p line_num will
 * not generate further interrupts or events.
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
 * @returns 0 on success, -EINVAL if @p line is invalid
 */
int stm32_exti_clear_pending(uint32_t line_num);

/**
 * @brief Generates SW interrupt for specified EXTI line number
 *
 * @param line_num	EXTI line number
 * @returns 0 on success, -EINVAL if @p line is invalid
 */
int stm32_exti_sw_interrupt(uint32_t line_num);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_EXTI_STM32_H_ */
