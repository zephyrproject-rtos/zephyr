/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RX group interrupt controller driver API
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RENESAS_RX_ICUM_GRP_IRQ_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RENESAS_RX_ICUM_GRP_IRQ_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>

/**
 * @brief Enable a group interrupt factor.
 *
 * Sets the factor's enable bit in the group interrupt request enable register (GENxx) and
 * enables the group interrupt vector in the ICU. The callback of the factor must be registered
 * with rx_icum_grp_irq_callback_set() first.
 *
 * @param dev Group interrupt controller device.
 * @param factor Interrupt factor (bit position in the group), 0 to 31.
 *
 * @retval 0 On success.
 * @retval -EINVAL @p factor is out of range, the vector of @p dev is not a group interrupt, or
 *                 no callback is registered for @p factor.
 */
int rx_icum_grp_irq_enable(const struct device *dev, uint8_t factor);

/**
 * @brief Disable a group interrupt factor.
 *
 * Clears the factor's enable bit in the group interrupt request enable register (GENxx). The
 * group interrupt vector is disabled in the ICU only when no other factor of the group is
 * enabled.
 *
 * @param dev Group interrupt controller device.
 * @param factor Interrupt factor (bit position in the group), 0 to 31.
 *
 * @retval 0 On success.
 * @retval -EINVAL @p factor is out of range.
 */
int rx_icum_grp_irq_disable(const struct device *dev, uint8_t factor);

/**
 * @brief Register the callback of a group interrupt factor.
 *
 * Also sets the group interrupt priority from the devicetree and stores @p context, which can be
 * retrieved in the callback with R_FSP_IsrContextGet(). Call this before enabling the factor.
 *
 * @param dev Group interrupt controller device.
 * @param factor Interrupt factor (bit position in the group), 0 to 31.
 * @param callback Function called from the group interrupt handler when the factor triggers.
 * @param context User context passed to the callback.
 *
 * @retval 0 On success.
 * @retval -EINVAL @p factor is out of range, @p callback is NULL, the vector or priority of
 *                 @p dev is invalid, or the vector of @p dev is not a group interrupt.
 */
int rx_icum_grp_irq_callback_set(const struct device *dev, uint8_t factor, void (*callback)(void),
				 void *context);

/**
 * @brief Set or clear the enable bit of a group interrupt factor.
 *
 * Writes only the factor's bit in the group interrupt request enable register (GENxx). Unlike
 * rx_icum_grp_irq_enable() and rx_icum_grp_irq_disable(), it does not enable or disable the
 * group interrupt vector in the ICU.
 *
 * @param dev Group interrupt controller device.
 * @param factor Interrupt factor (bit position in the group), 0 to 31.
 * @param set true to set the enable bit, false to clear it.
 *
 * @retval 0 On success.
 * @retval -EINVAL @p factor is out of range.
 */
int rx_icum_grp_irq_gen_set(const struct device *dev, uint8_t factor, bool set);

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RENESAS_RX_ICUM_GRP_IRQ_H_ */
