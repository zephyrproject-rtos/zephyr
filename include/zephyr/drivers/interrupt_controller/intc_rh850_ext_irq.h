/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RH850 external interrupt controller interface
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RH850_EXT_IRQ_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RH850_EXT_IRQ_H_

#include <zephyr/device.h>

/** Trigger detection disabled */
#define RH850_EXT_IRQ_TRIG_DISABLE    0
/** Trigger on rising edge */
#define RH850_EXT_IRQ_TRIG_RISING     1
/** Trigger on falling edge */
#define RH850_EXT_IRQ_TRIG_FALLING    2
/** Trigger on both edges */
#define RH850_EXT_IRQ_TRIG_BOTH_EDGE  3
/** Trigger on low level */
#define RH850_EXT_IRQ_TRIG_LEVEL_LOW  4
/** Trigger on high level */
#define RH850_EXT_IRQ_TRIG_LEVEL_HIGH 5

/** RH850 external interrupt callback */
typedef void (*intc_rh850_ext_irq_callback_t)(void *arg);

/**
 * @brief Enable external interrupt for specified channel.
 *
 * @param dev Pointer to interrupt controller instance.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid argument.
 * @retval -EACCES Channel is not open.
 * @retval -ENOTSUP Interrupt is disabled in the BSP configuration.
 * @retval -EIO Other FSP error.
 */
int intc_rh850_ext_irq_enable(const struct device *dev);

/**
 * @brief Disable external interrupt for specified channel.
 *
 * @param dev Pointer to interrupt controller instance.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid argument.
 * @retval -EACCES Channel is not open.
 * @retval -ENOTSUP Interrupt is disabled in the BSP configuration.
 * @retval -EIO Other FSP error.
 */
int intc_rh850_ext_irq_disable(const struct device *dev);

/**
 * @brief Set the user callback.
 *
 * @param dev Pointer to interrupt controller instance.
 * @param cb Callback to set.
 * @param arg User data passed to the callback.
 *
 * @retval 0 Success.
 */
int intc_rh850_ext_irq_set_callback(const struct device *dev, intc_rh850_ext_irq_callback_t cb,
				      void *arg);

/**
 * @brief Change the trigger type of the specified channel.
 *
 * @param dev Pointer to interrupt controller instance.
 * @param trig Trigger type, one of the RH850_EXT_IRQ_TRIG_* values.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid argument.
 * @retval -EALREADY Channel is already open.
 * @retval -ENODEV Channel is not present.
 * @retval -ENOTSUP Trigger type is not supported.
 * @retval -EIO Other FSP error.
 */
int intc_rh850_ext_irq_set_type(const struct device *dev, uint8_t trig);

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RH850_EXT_IRQ_H_ */
