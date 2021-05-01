/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2021 Tokita, Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for External interrupt/event controller in GD32 MCUs
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_EXTI_GD32_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_EXTI_GD32_H_

#include <zephyr/types.h>

/* device name */
#define GD32_EXTI_NAME "gd32-exti"

/**
 * @brief enable EXTI interrupt for specific line
 *
 * @param line EXTI# line
 */
void gd32_exti_enable(int line);

/**
 * @brief disable EXTI interrupt for specific line
 *
 * @param line EXTI# line
 */
void gd32_exti_disable(int line);

/**
 * @brief set EXTI interrupt line triggers
 *
 * @param line EXTI# line
 * @param trg  OR'ed gd32_exti_trigger flags
 */
void gd32_exti_trigger(int line, int trg);

/* callback for exti interrupt */
typedef void (*gd32_exti_callback_t) (int line, void *user);

/**
 * @brief set EXTI interrupt callback
 *
 * @param line EXI# line
 * @param cb   user callback
 * @param arg  user arg
 */
int gd32_exti_set_callback(int line, gd32_exti_callback_t cb, void *data);

/**
 * @brief unset EXTI interrupt callback
 *
 * @param line EXI# line
 */
void gd32_exti_unset_callback(int line);

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_EXTI_GD32_H_ */
