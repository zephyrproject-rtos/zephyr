/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Hazard3 interrupt controller driver API
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_HAZARD3_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_HAZARD3_H_

/**
 * @brief Enable an IRQ line
 *
 * @param irq IRQ line number
 */
void intc_hazard3_irq_enable(unsigned int irq);

/**
 * @brief Disable an IRQ line
 *
 * @param irq IRQ line number
 */
void intc_hazard3_irq_disable(unsigned int irq);

/**
 * @brief Get the enable state of an IRQ line
 *
 * @param irq IRQ line number
 *
 * @return 1 if the IRQ line is enabled, 0 otherwise
 */
int intc_hazard3_irq_is_enabled(unsigned int irq);

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_HAZARD3_H_ */
