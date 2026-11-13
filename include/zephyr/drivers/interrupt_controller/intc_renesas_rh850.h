/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RH850 interrupt controller (INTC1/INTC2/FEINC) interface
 *
 * Interrupt numbering: INTC1 channels come first, then INTC2 channels, then
 * FEINC channels. INTC1 and FEINC operations act on the instance owned by
 * the calling PE.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_RENESAS_RH850_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_RENESAS_RH850_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enable an interrupt line.
 *
 * Selects table reference for INTC1/INTC2 channels, binds INTC2 channels to
 * the calling PE and unmasks the channel.
 *
 * @param irq Interrupt number.
 */
void intc_rh850_irq_enable(unsigned int irq);

/**
 * @brief Disable an interrupt line.
 *
 * @param irq Interrupt number.
 */
void intc_rh850_irq_disable(unsigned int irq);

/**
 * @brief Check whether an interrupt line is enabled.
 *
 * @param irq Interrupt number.
 *
 * @retval true The line is unmasked.
 * @retval false The line is masked or is not handled by this controller.
 */
bool intc_rh850_irq_is_enabled(unsigned int irq);

/**
 * @brief Set the priority of an interrupt line.
 *
 * Has no effect on FEINC channels, which have no priority setting.
 *
 * @param irq Interrupt number.
 * @param prio Priority level, 0 being the highest.
 * @param flags Architecture-specific flags (unused).
 */
void intc_rh850_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_RENESAS_RH850_H_ */
