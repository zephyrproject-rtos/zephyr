/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief GRLIB IRQMP interrupt controller driver API
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_IRQMP_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_IRQMP_H_

/**
 * @brief Enable an interrupt source
 *
 * @param source Interrupt source number
 */
void intc_irqmp_irq_enable(unsigned int source);

/**
 * @brief Disable an interrupt source
 *
 * @param source Interrupt source number
 */
void intc_irqmp_irq_disable(unsigned int source);

/**
 * @brief Get the enable state of an interrupt source
 *
 * @param source Interrupt source number
 *
 * @return 1 if the interrupt source is enabled, 0 otherwise
 */
int intc_irqmp_irq_is_enabled(unsigned int source);

/**
 * @brief Get the interrupt source behind a processor interrupt request
 *
 * Acknowledges interrupt request level @a irl and returns the actual
 * interrupt source, taking IRQMP extended interrupts into account.
 *
 * @param irl Processor interrupt request level
 *
 * @return Interrupt source number
 */
int intc_irqmp_get_source(int irl);

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_IRQMP_H_ */
