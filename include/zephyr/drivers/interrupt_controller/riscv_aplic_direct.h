/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RISC-V APLIC direct delivery mode driver API
 *
 * This header provides the API for the RISC-V Advanced Platform-Level
 * Interrupt Controller (APLIC) operating in direct delivery mode.
 * In direct mode, interrupts are delivered directly to the hart's
 * MEXT interrupt line rather than via MSI.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_RISCV_APLIC_DIRECT_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_RISCV_APLIC_DIRECT_H_

#include <zephyr/device.h>
#include <zephyr/types.h>
#include <zephyr/arch/riscv/csr.h>

/**
 * @brief Enable or disable the APLIC delivery mode
 *
 * Controls the delivery mode bit in the domain configuration register.
 *
 * @param dev APLIC device
 * @param enable true to enable direct delivery mode, false to disable
 * @return 0 on success, negative error code on failure
 */
int riscv_aplic_direct_mode_enable(const struct device *dev, bool enable);

/**
 * @brief Check if a riscv APLIC-specific interrupt line is enabled
 *
 * @param local_irq Local IRQ number to check
 *
 * @return 1 if enabled, 0 otherwise
 */
int riscv_aplic_is_enabled(uint32_t local_irq);

/**
 * @brief Configure the priority for a specified interrupt source
 *        for a domain with direct delivery mode enabled.
 *
 * @param local_irq Local IRQ whose target register will be configured
 * @param prio Priority value to set
 * @return 0 on success, negative error code on failure
 */
int riscv_aplic_set_priority(uint32_t local_irq, uint32_t prio);

/**
 * @brief Get active interrupt ID
 *
 * @note Should be called with interrupt locked
 *
 * @return Returns the ID of an active interrupt
 */
unsigned int riscv_aplic_get_saved_irq(void);

/**
 * @brief Get active interrupt controller device
 *
 * @note Should be called with interrupt locked
 *
 * @return Returns device pointer of the active interrupt device
 */
const struct device *riscv_aplic_get_saved_dev(void);

/**
 * @brief IRQ handler for APLIC direct-mode interrupts
 *
 * @param dev APLIC device
 */
void aplic_irq_handler(const struct device *dev);

/**
 * @brief Initialization for APLIC with direct delivery mode enabled
 *
 * @param dev APLIC device
 *
 * @return 0 on success, negative error code on failure
 */
int aplic_direct_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RISCV_APLIC_DIRECT_H_ */
