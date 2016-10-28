/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @brief Driver for External interrupt/event controller in STM32 MCUs
 *
 * Based on reference manuals:
 *   RM0008 Reference Manual: STM32F101xx, STM32F102xx, STM32F103xx, STM32F105xx
 *   and STM32F107xx advanced ARM-based 32-bit MCUs
 * and
 *   RM0368 Reference manual STM32F401xB/C and STM32F401xD/E
 *   advanced ARM-based 32-bit MCUs
 *
 * Chapter 10.2: External interrupt/event controller (EXTI)
 *
 */

#ifndef _STM32_EXTI_H_
#define _STM32_EXTI_H_

#include <stdint.h>

/* device name */
#define STM32_EXTI_NAME "stm32-exti"

/**
 * @brief enable EXTI interrupt for specific line
 *
 * @param line EXTI# line
 */
void stm32_exti_enable(int line);

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
void stm32_exti_set_callback(int line, stm32_exti_callback_t cb, void *data);

/**
 * @brief unset EXTI interrupt callback
 *
 * @param line EXI# line
 */
void stm32_exti_unset_callback(int line);

#endif /* _STM32_EXTI_H_ */
