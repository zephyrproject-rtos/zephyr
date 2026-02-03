/*
 * Copyright (c) 2025 Synopsys, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RISC-V AIA (Advanced Interrupt Architecture) unified API
 *
 * This header provides a unified API for the RISC-V Advanced Interrupt
 * Architecture (AIA), wrapping both APLIC (Advanced Platform-Level
 * Interrupt Controller) and IMSIC (Incoming Message-Signaled Interrupt
 * Controller) functionality.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_RISCV_AIA_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_RISCV_AIA_H_

#include <zephyr/device.h>
#include <zephyr/types.h>
#include <zephyr/drivers/interrupt_controller/riscv_aplic.h>
#include <zephyr/drivers/interrupt_controller/riscv_imsic.h>

/**
 * @brief Check if interrupt source is an APLIC source
 *
 * Determines whether the given interrupt source number corresponds to
 * an APLIC external interrupt source or a local interrupt.
 *
 * @param src Interrupt source number
 * @return true if source is an APLIC source, false otherwise
 */
bool riscv_aia_is_aplic_source(uint32_t src);

/**
 * @brief Enable an APLIC interrupt source
 *
 * Enables the specified APLIC interrupt source. Uses 1:1 EIID mapping.
 *
 * @param src Interrupt source number
 */
void riscv_aia_irq_enable(uint32_t src);

/**
 * @brief Disable an APLIC interrupt source
 *
 * Disables the specified APLIC interrupt source.
 *
 * @param src Interrupt source number
 */
void riscv_aia_irq_disable(uint32_t src);

/**
 * @brief Check if an APLIC interrupt source is enabled
 *
 * @param src Interrupt source number
 * @return 1 if enabled, 0 if disabled
 */
int riscv_aia_irq_is_enabled(uint32_t src);

/**
 * @brief Set priority for an interrupt source
 *
 * Sets the priority level for the specified interrupt source in the IMSIC.
 *
 * @param src Interrupt source number
 * @param prio Priority level
 */
void riscv_aia_set_priority(uint32_t src, uint32_t prio);

/**
 * @brief Get the AIA coordinator device
 *
 * Returns the device structure for the AIA coordinator driver.
 *
 * @return Pointer to the AIA device, or NULL if not available
 */
const struct device *riscv_aia_get_dev(void);

/**
 * @brief Configure an interrupt source mode
 *
 * Configures the trigger mode for an APLIC interrupt source.
 *
 * @param src Interrupt source number
 * @param mode Source mode (e.g., APLIC_SM_EDGE_RISE, APLIC_SM_LEVEL_HIGH)
 */
void riscv_aia_config_source(unsigned int src, uint32_t mode);

/**
 * @brief Route an interrupt source to a specific hart
 *
 * Configures the APLIC to route the specified interrupt source to
 * a target hart with the given EIID (External Interrupt Identity).
 *
 * @param src Interrupt source number
 * @param hart Target hart index
 * @param eiid External Interrupt Identity for the target IMSIC
 */
void riscv_aia_route_to_hart(unsigned int src, uint32_t hart, uint32_t eiid);

/**
 * @brief Enable an interrupt source in the APLIC
 *
 * Enables the specified interrupt source in the APLIC domain.
 *
 * @param src Interrupt source number
 */
void riscv_aia_enable_source(unsigned int src);

/**
 * @brief Inject a software MSI to a hart
 *
 * Triggers a software-generated MSI (Message-Signaled Interrupt) to
 * the specified hart with the given EIID.
 *
 * @param hart Target hart index
 * @param eiid External Interrupt Identity
 */
void riscv_aia_inject_msi(uint32_t hart, uint32_t eiid);

#endif
