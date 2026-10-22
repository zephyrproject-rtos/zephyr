/*
 * Copyright (c) 2026 Anuj Deshpande
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver for the C-DAC THEJAS32 interrupt controller
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_THEJAS32_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_THEJAS32_H_

#include <stdint.h>

/**
 * @brief Enable an interrupt source
 *
 * @param irq Multi-level encoded interrupt ID
 */
void thejas32_intc_irq_enable(uint32_t irq);

/**
 * @brief Disable an interrupt source
 *
 * @param irq Multi-level encoded interrupt ID
 */
void thejas32_intc_irq_disable(uint32_t irq);

/**
 * @brief Check if an interrupt source is enabled
 *
 * @param irq Multi-level encoded interrupt ID
 * @return Non-zero if the interrupt is enabled, 0 otherwise
 */
int thejas32_intc_irq_is_enabled(uint32_t irq);

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_THEJAS32_H_ */
