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
#include <stm32_ll_exti.h>

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
 * @param line	EXTI line on which interrupt occurred
 * @param user	@p data provided to @ref stm32_exti_enable
 *
 * @note This callback is invoked in ISR context.
 */
typedef void (*stm32_exti_irq_cb_t)(uint32_t line, void *user);

/**
 * @brief Enable EXTI line.
 * @note Depending on mode either interrupt or event for corresponding line
 *       will be enabled. IRQ callback will be	set if provided.
 *
 * @param line		EXTI line
 * @param trigger	EXTI trigger type (see @ref stm32_exti_trigger_type)
 * @param mode		EXTI mode (see @ref stm32_exti_mode)
 * @param cb		Interrupt callback function
 * @param user	Custom user data for usage by the callback
 * @returns 0 on success, -EBUSY if a callback is already set for @p line,
 *                        -EINVAL if @p line is invalid
 */
int stm32_exti_enable(uint32_t line, stm32_exti_trigger_type trigger,
		stm32_exti_mode mode, stm32_exti_irq_cb_t cb, void *user);

/**
 * @brief Disable EXTI line.
 *        This function is doing following steps in the exact order:
 *           - Set EXTI line mode to STM32_EXTI_MODE_NONE
 *           - Set EXTI line trigger ti STM32_EXTI_TRIG_NONE
 *           - Remove EXTI line callback
 * @note Interrupt and event for given line number will be disabled and trigger
 *       will be set to none. Also interrupt callback will be removed.
 *
 * @param line	EXTI line
 * @returns 0 on success, -EINVAL if @p line is invalid
 */
int stm32_exti_disable(uint32_t line);

/**
 * @brief Checks interrupt pending bit for specified EXTI line
 *
 * @param line EXTI line number
 */
int stm32_exti_is_pending(uint32_t line);

/**
 * @brief Clears interrupt pending bit for specified EXTI line
 *
 * @param line	EXTI line
 * @returns 0 - no pending signal on line, not 0 - pending signal on line
 */
int stm32_exti_clear_pending(uint32_t line);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_EXTI_STM32_H_ */
