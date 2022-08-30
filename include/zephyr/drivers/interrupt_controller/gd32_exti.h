/*
 * Copyright (c) 2021 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_GD32_EXTI_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_GD32_EXTI_H_

#include <stdint.h>

#include <zephyr/sys/util_macro.h>

/**
 * @name EXTI trigger modes.
 * @anchor GD32_EXTI_TRIG
 * @{
 */

/** No trigger */
#define GD32_EXTI_TRIG_NONE 0U
/** Trigger on rising edge */
#define GD32_EXTI_TRIG_RISING BIT(0)
/** Trigger on falling edge */
#define GD32_EXTI_TRIG_FALLING BIT(1)
/** Trigger on rising and falling edge */
#define GD32_EXTI_TRIG_BOTH (GD32_EXTI_TRIG_RISING | GD32_EXTI_TRIG_FALLING)

/** @} */

/** Callback for EXTI interrupt. */
typedef void (*gd32_exti_cb_t)(uint8_t line, void *user);

/**
 * @brief Enable EXTI interrupt for the given line.
 *
 * @param line EXTI line.
 */
void gd32_exti_enable(uint8_t line);

/**
 * @brief Disable EXTI interrupt for the given line.
 *
 * @param line EXTI line.
 */
void gd32_exti_disable(uint8_t line);

/**
 * @brief Configure EXTI interrupt trigger mode for the given line.
 *
 * @param line EXTI line.
 * @param trigger Trigger mode (see @ref GD32_EXTI_TRIG).
 */
void gd32_exti_trigger(uint8_t line, uint8_t trigger);

/**
 * @brief Configure EXTI interrupt callback.
 *
 * @param line EXTI line.
 * @param cb Callback (NULL to disable).
 * @param user User data (optional).
 *
 * @retval 0 On success.
 * @retval -EALREADY If callback is already set and @p cb is not NULL.
 */
int gd32_exti_configure(uint8_t line, gd32_exti_cb_t cb, void *user);

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_GD32_EXTI_H_ */
