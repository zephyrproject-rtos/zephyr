/*
 * Copyright (c) 2024 Abe Kohandel <abe.kohandel@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TI AM335x series interrupt controller APIs
 */

#ifndef __ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_TI_AM335X_H_
#define __ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_TI_AM335X_H_

#include <zephyr/device.h>

/**
 * @brief Initialize the interrupt controller
 *
 * Disable all interrupts as part of initializing the controller
 */
void intc_ti_am335x_irq_init(void);

/**
 * @brief Enable provided IRQ
 *
 * @param irq IRQ to enable
 */
void intc_ti_am335x_irq_enable(unsigned int irq);

/**
 * @brief Disable provided IRQ
 *
 * @param irq IRQ to disable
 */
void intc_ti_am335x_irq_disable(unsigned int irq);

/**
 * @brief Get enable status of an IRQ
 *
 * @param irq IRQ to check
 *
 * @return 0 if disables, non-zero if enabled
 */
int intc_ti_am335x_irq_is_enabled(unsigned int irq);

/**
 * @brief Set IRQ priority
 *
 * @param irq   IRQ to configure
 * @param prio  priority to set
 * @param flags unused parameter
 */
void intc_ti_am335x_irq_priority_set(unsigned int irq, unsigned int prio, unsigned int flags);

/**
 * @brief Get currently active IRQ
 *
 * In order to support nested interrupts this function also disables the active interrupt by masking
 * it and requests a new IRQ generation from the interrupt controller.
 *
 * @return IRQ number of the active IRQ
 */
unsigned int intc_ti_am335x_irq_get_active(void);

/**
 * @brief End of interrupt
 *
 * This method enables the provided IRQ by unmasking the interrupt.
 *
 * @param irq IRQ number for interrupt that is finished processing
 */
void intc_ti_am335x_irq_eoi(unsigned int irq);

#endif /* __ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_TI_AM335X_H_ */
