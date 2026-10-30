/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RH850 interrupt controller (INTC1 and INTC2) interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_RENESAS_RH850_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_RENESAS_RH850_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Set the priority of a channel and select the direct vector method.
 *
 * @param irq Channel number.
 * @param prio Priority, 0 (highest) to 15.
 */
void intc_rh850_irq_set_priority(unsigned int irq, unsigned int prio);

/**
 * @brief Unmask a channel.
 *
 * @param irq Channel number.
 */
void intc_rh850_irq_enable(unsigned int irq);

/**
 * @brief Mask a channel.
 *
 * @param irq Channel number.
 */
void intc_rh850_irq_disable(unsigned int irq);

/**
 * @brief Check whether a channel is unmasked.
 *
 * @param irq Channel number.
 *
 * @retval true The channel is unmasked.
 * @retval false The channel is masked.
 */
bool intc_rh850_irq_is_enabled(unsigned int irq);

/**
 * @brief Request an interrupt on an edge-detected channel by software.
 *
 * @param irq Channel number.
 */
void intc_rh850_irq_raise(unsigned int irq);

/**
 * @brief Check the request flag of a channel.
 *
 * @param irq Channel number.
 *
 * @retval true A request is pending.
 * @retval false No request is pending.
 */
bool intc_rh850_irq_is_pending(unsigned int irq);

/**
 * @brief Withdraw a pending request of an edge-detected channel.
 *
 * @param irq Channel number.
 */
void intc_rh850_irq_clear_pending(unsigned int irq);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_RENESAS_RH850_H_ */
