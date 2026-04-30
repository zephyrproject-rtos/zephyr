/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RISC-V AIA (Advanced Interrupt Architecture) unified coordinator API.
 *
 * Provides a single API wrapping APLIC and IMSIC for Zephyr's interrupt
 * management. The coordinator is exposed to Zephyr as a PLIC-like second-level
 * interrupt controller; APLIC and IMSIC remain internal implementation details.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_RISCV_AIA_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_RISCV_AIA_H_

#include <zephyr/device.h>
#include <zephyr/types.h>
#include <zephyr/drivers/interrupt_controller/riscv_aplic.h>
#include <zephyr/drivers/interrupt_controller/riscv_imsic.h>

/**
 * @brief Enable an AIA interrupt source.
 *
 * Enables the EIID in IMSIC and configures/enables the APLIC source.
 *
 * @param irq Multi-level encoded interrupt ID.
 */
void riscv_aia_irq_enable(uint32_t irq);

/**
 * @brief Disable an AIA interrupt source.
 *
 * Disables both the IMSIC EIID and the APLIC source.
 *
 * @param irq Multi-level encoded interrupt ID.
 */
void riscv_aia_irq_disable(uint32_t irq);

/**
 * @brief Check if an AIA interrupt source is enabled.
 *
 * @param irq Multi-level encoded interrupt ID.
 * @return Non-zero if enabled, 0 if disabled.
 */
int riscv_aia_irq_is_enabled(uint32_t irq);

/**
 * @brief Set priority for an AIA interrupt source.
 *
 * APLIC-MSI mode has no per-source priority registers. Priority is
 * handled via IMSIC EITHRESHOLD or implicit EIID ordering.
 *
 * @param irq Multi-level encoded interrupt ID.
 * @param prio Priority value (currently ignored in MSI mode).
 */
void riscv_aia_set_priority(uint32_t irq, uint32_t prio);

/**
 * @brief Get the underlying APLIC device.
 *
 * @return Pointer to the APLIC device.
 */
static inline const struct device *riscv_aia_get_dev(void)
{
	return DEVICE_DT_GET_ANY(riscv_aplic);
}

/**
 * @brief Configure an APLIC source mode.
 *
 * @param irq Multi-level encoded interrupt ID.
 * @param mode Source mode (trigger type) per RISC-V AIA spec section 4.5.2.
 */
void riscv_aia_config_source(uint32_t irq, uint32_t mode);

#if defined(CONFIG_RISCV_APLIC_MSI) || defined(__DOXYGEN__)
/**
 * @brief Route an APLIC source to a specific hart.
 *
 * @param irq Multi-level encoded interrupt ID.
 * @param hart Target hart index.
 * @param eiid EIID to deliver to the target hart's IMSIC.
 */
void riscv_aia_route_to_hart(uint32_t irq, uint32_t hart, uint32_t eiid);

/**
 * @brief Inject a software MSI via the APLIC GENMSI register.
 *
 * @param hart Target hart index.
 * @param eiid EIID to inject.
 */
void riscv_aia_inject_msi(uint32_t hart, uint32_t eiid);
#endif /* CONFIG_RISCV_APLIC_MSI */

/**
 * @brief Enable an APLIC source using the convenience wrapper.
 *
 * @param irq Multi-level encoded interrupt ID.
 */
void riscv_aia_enable_source(uint32_t irq);

/**
 * @brief Dispatch an IMSIC EIID through the AIA second-level ISR table.
 *
 * @param eiid IMSIC external interrupt identity.
 */
void riscv_aia_dispatch_eiid(uint32_t eiid);

#endif
