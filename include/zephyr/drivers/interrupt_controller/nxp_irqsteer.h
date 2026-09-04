/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief NXP IRQSTEER interrupt aggregator driver API
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_NXP_IRQSTEER_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_NXP_IRQSTEER_H_

#include <zephyr/types.h>

/**
 * @brief Enable a multi-level IRQ line
 *
 * @param irq Multi-level encoded IRQ number
 */
void nxp_irqstr_irq_enable(uint32_t irq);

/**
 * @brief Disable a multi-level IRQ line
 *
 * @param irq Multi-level encoded IRQ number
 */
void nxp_irqstr_irq_disable(uint32_t irq);

/**
 * @brief Get the enable state of a multi-level IRQ line
 *
 * @param irq Multi-level encoded IRQ number
 *
 * @return 1 if the IRQ line is enabled, 0 otherwise
 */
int nxp_irqstr_irq_is_enabled(unsigned int irq);

/**
 * @brief Set the priority of the parent line of a multi-level IRQ
 *
 * @param irq Multi-level encoded IRQ number
 * @param prio Interrupt priority
 * @param flags Architecture-specific IRQ configuration flags
 */
void nxp_irqstr_irq_priority_set(unsigned int irq, unsigned int prio, unsigned int flags);

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_NXP_IRQSTEER_H_ */
