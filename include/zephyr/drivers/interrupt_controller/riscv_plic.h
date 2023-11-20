/*
 * Copyright (c) 2022 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver for Platform Level Interrupt Controller (PLIC)
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RISCV_PLIC_H_
#define ZEPHYR_INCLUDE_DRIVERS_RISCV_PLIC_H_

#include <zephyr/device.h>

/**
 * @brief Enable interrupt
 *
 * @param irq Multi-level encoded interrupt ID
 */
void riscv_plic_irq_enable(uint32_t irq);

/**
 * @brief Disable interrupt
 *
 * @param irq Multi-level encoded interrupt ID
 */
void riscv_plic_irq_disable(uint32_t irq);

/**
 * @brief Check if an interrupt is enabled
 *
 * @param irq Multi-level encoded interrupt ID
 * @return Returns true if interrupt is enabled, false otherwise
 */
int riscv_plic_irq_is_enabled(uint32_t irq);

/**
 * @brief Set interrupt priority
 *
 * @param irq Multi-level encoded interrupt ID
 * @param prio interrupt priority
 */
void riscv_plic_set_priority(uint32_t irq, uint32_t prio);

/**
 * @brief Get active interrupt ID
 *
 * @return Returns the ID of an active interrupt
 */
unsigned int riscv_plic_get_irq(void);

/**
 * @brief Get active interrupt controller device
 *
 * @return Returns device pointer of the active interrupt device
 */
const struct device *riscv_plic_get_dev(void);

#endif /* ZEPHYR_INCLUDE_DRIVERS_RISCV_PLIC_H_ */
