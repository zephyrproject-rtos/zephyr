/*
 * Copyright (c) 2022 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver for Core-Local Interrupt Controller (CLIC)
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RISCV_CLIC_H_
#define ZEPHYR_INCLUDE_DRIVERS_RISCV_CLIC_H_

/**
 * @brief Enable interrupt
 *
 * @param irq interrupt ID
 */
void riscv_clic_irq_enable(uint32_t irq);

/**
 * @brief Disable interrupt
 *
 * @param irq interrupt ID
 */
void riscv_clic_irq_disable(uint32_t irq);

/**
 * @brief Check if an interrupt is enabled
 *
 * @param irq interrupt ID
 * @return Returns true if interrupt is enabled, false otherwise
 */
int riscv_clic_irq_is_enabled(uint32_t irq);

/**
 * @brief Set interrupt priority
 *
 * @param irq interrupt ID
 * @param prio interrupt priority
 * @param flags interrupt flags
 */
void riscv_clic_irq_priority_set(uint32_t irq, uint32_t prio, uint32_t flags);

#endif /* ZEPHYR_INCLUDE_DRIVERS_RISCV_CLIC_H_ */
