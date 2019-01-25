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

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_EXTI_STM32_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_EXTI_STM32_H_

#include <zephyr/types.h>

/* device name */
#define STM32_EXTI_NAME "stm32-exti"

/**
 * @brief enable EXTI interrupt for specific line
 *
 * @param line EXTI# line
 */
int stm32_exti_enable(int line);

/**
 * @brief disable EXTI interrupt for specific line
 *
 * @param line EXTI# line
 */
void stm32_exti_disable(int line);

/**
 * @brief EXTI trigger flags
 */
enum stm32_exti_trigger {
	/* trigger on rising edge */
	STM32_EXTI_TRIG_RISING  = 0x1,
	/* trigger on falling endge */
	STM32_EXTI_TRIG_FALLING = 0x2,
};

/**
 * @brief set EXTI interrupt line triggers
 *
 * @param line EXTI# line
 * @param trg  OR'ed stm32_exti_trigger flags
 */
void stm32_exti_trigger(int line, int trg);

/* callback for exti interrupt */
typedef void (*stm32_exti_callback_t) (int line, void *user);

/**
 * @brief set EXTI interrupt callback
 *
 * @param line EXI# line
 * @param cb   user callback
 * @param arg  user arg
 */
int stm32_exti_set_callback(int line, int port, stm32_exti_callback_t cb,
				void *data);

/**
 * @brief unset EXTI interrupt callback
 *
 * @param line EXI# line
 */
void stm32_exti_unset_callback(int line);

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_EXTI_STM32_H_ */
