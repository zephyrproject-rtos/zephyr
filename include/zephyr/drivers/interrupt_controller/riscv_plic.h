/*
 * Copyright (c) 2022 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver for Platform Level Interrupt Controller (PLIC)
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_RISCV_PLIC_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_RISCV_PLIC_H_

#include <zephyr/device.h>

/**
 * @brief Mark interrupt as completed
 *
 * @param irq IRQ number to claim as completed
 */
void riscv_plic_irq_complete(uint32_t irq);

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
 * @brief Set priority of a riscv PLIC-specific interrupt line
 *
 * This routine set the priority of a RISCV PLIC-specific interrupt line.
 * riscv_plic_irq_set_prio is called by riscv arch_irq_priority_set to set
 * the priority of an interrupt whenever CONFIG_RISCV_HAS_PLIC variable is set.
 *
 * @param irq IRQ number for which to set priority
 * @param priority Priority of IRQ to set to
 * @param flags Architecture specific flags, defined in <zephyr/arch/riscv/irq.h>
 */
void riscv_plic_set_priority(uint32_t irq, uint32_t priority, uint32_t flags);

/**
 * @brief Set IRQ affinity.
 *
 * @param irq IRQ line.
 * @param cpumask CPU bit mask.
 *
 * @return 0 if success, negative errno value otherwise
 */
int riscv_plic_irq_set_affinity(uint32_t irq, uint32_t cpumask);

/**
 * @brief Set interrupt as pending
 *
 * @param irq Multi-level encoded interrupt ID
 */
void riscv_plic_irq_set_pending(uint32_t irq);

/**
 * @brief Get active interrupt ID
 *
 * @note Should be called with interrupt locked
 *
 * @return Returns the ID of an active interrupt
 */
unsigned int riscv_plic_get_irq(void);

/**
 * @brief Get active interrupt controller device
 *
 * @note Should be called with interrupt locked
 *
 * @return Returns device pointer of the active interrupt device
 */
const struct device *riscv_plic_get_dev(void);

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_RISCV_PLIC_H_ */
