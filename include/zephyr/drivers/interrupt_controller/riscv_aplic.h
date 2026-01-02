/*
 * Copyright (c) 2025 Synopsys, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RISC-V APLIC (Advanced Platform-Level Interrupt Controller) driver API
 *
 * This header provides the API and register definitions for the RISC-V
 * Advanced Platform-Level Interrupt Controller (APLIC) in MSI delivery mode.
 * The APLIC is part of the RISC-V Advanced Interrupt Architecture (AIA).
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_RISCV_APLIC_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_RISCV_APLIC_H_

#include <zephyr/device.h>
#include <zephyr/types.h>

/**
 * @name APLIC Register Offsets
 * APLIC (MSI mode) register offsets as defined in the AIA specification.
 * @{
 */
/** @brief Domain configuration register offset */
#define APLIC_DOMAINCFG      0x0000
/** @brief Source configuration registers base offset */
#define APLIC_SOURCECFG_BASE 0x0004
/** @brief Set interrupt pending bitmap base offset */
#define APLIC_SETIP_BASE     0x1C00
/** @brief Set interrupt pending by number register offset */
#define APLIC_SETIPNUM       0x1CDC
/** @brief Clear interrupt pending bitmap base offset */
#define APLIC_CLRIP_BASE     0x1D00
/** @brief Clear interrupt pending by number register offset */
#define APLIC_CLRIPNUM       0x1DDC
/** @brief Set interrupt enable bitmap base offset */
#define APLIC_SETIE_BASE     0x1E00
/** @brief Set interrupt enable by number register offset */
#define APLIC_SETIENUM       0x1EDC
/** @brief Clear interrupt enable bitmap base offset */
#define APLIC_CLRIE_BASE     0x1F00
/** @brief Clear interrupt enable by number register offset */
#define APLIC_CLRIENUM       0x1FDC
/** @brief MSI address configuration register offset */
#define APLIC_MSIADDRCFG     0x1BC0
/** @brief MSI address configuration high register offset */
#define APLIC_MSIADDRCFGH    0x1BC4
/** @brief Supervisor MSI address configuration register offset */
#define APLIC_SMSIADDRCFG    0x1BC8
/** @brief Supervisor MSI address configuration high register offset */
#define APLIC_SMSIADDRCFGH   0x1BCC
/** @brief Generate MSI register offset */
#define APLIC_GENMSI         0x3000
/** @brief Target registers base offset */
#define APLIC_TARGET_BASE    0x3004
/** @} */

/**
 * @name APLIC Domain Configuration Bits
 * Bit definitions for the APLIC domain configuration register.
 * @{
 */
/** @brief Interrupt enable bit in domaincfg */
#define APLIC_DOMAINCFG_IE BIT(8)
/** @brief Delivery mode bit in domaincfg (1=MSI mode) */
#define APLIC_DOMAINCFG_DM BIT(2)
/** @brief Big endian bit in domaincfg */
#define APLIC_DOMAINCFG_BE BIT(0)
/** @} */

/**
 * @name APLIC MSI Address Configuration Fields
 * Bit field definitions for MSIADDRCFGH register used by APLIC
 * to calculate per-hart MSI target addresses.
 * @{
 */
/** @brief Lock bit position in MSIADDRCFGH */
#define APLIC_MSIADDRCFGH_L_BIT      31
/** @brief Higher Hart Index Shift field position */
#define APLIC_MSIADDRCFGH_HHXS_SHIFT 24
/** @brief Higher Hart Index Shift field mask */
#define APLIC_MSIADDRCFGH_HHXS_MASK  0x1F
/** @brief Lower Hart Index Shift field position */
#define APLIC_MSIADDRCFGH_LHXS_SHIFT 20
/** @brief Lower Hart Index Shift field mask */
#define APLIC_MSIADDRCFGH_LHXS_MASK  0x7
/** @brief Higher Hart Index Width field position */
#define APLIC_MSIADDRCFGH_HHXW_SHIFT 16
/** @brief Higher Hart Index Width field mask */
#define APLIC_MSIADDRCFGH_HHXW_MASK  0x7
/** @brief Lower Hart Index Width field position */
#define APLIC_MSIADDRCFGH_LHXW_SHIFT 12
/** @brief Lower Hart Index Width field mask */
#define APLIC_MSIADDRCFGH_LHXW_MASK  0xF
/** @brief Base address PPN field mask (upper address bits) */
#define APLIC_MSIADDRCFGH_BAPPN_MASK 0xFFF
/** @} */

/**
 * @name APLIC Source Configuration Fields
 * Bit field definitions for source configuration registers.
 * @{
 */
/** @brief Delegate bit position in sourcecfg */
#define APLIC_SOURCECFG_D_BIT   10
/** @brief Source mode field mask (bits 2:0) */
#define APLIC_SOURCECFG_SM_MASK 0x7
/** @brief Source mode: inactive */
#define APLIC_SM_INACTIVE       0x0
/** @brief Source mode: detached (delegated to child domain) */
#define APLIC_SM_DETACHED       0x1
/** @brief Source mode: rising edge triggered */
#define APLIC_SM_EDGE_RISE      0x4
/** @brief Source mode: falling edge triggered */
#define APLIC_SM_EDGE_FALL      0x5
/** @brief Source mode: active high level triggered */
#define APLIC_SM_LEVEL_HIGH     0x6
/** @brief Source mode: active low level triggered */
#define APLIC_SM_LEVEL_LOW      0x7
/** @} */

/**
 * @name APLIC Target Register Fields
 * Bit field definitions for TARGET registers (MSI routing).
 * @{
 */
/** @brief Hart index field shift in TARGET register */
#define APLIC_TARGET_HART_SHIFT 18
/** @brief Hart index field mask (14-bit, bits 31:18) */
#define APLIC_TARGET_HART_MASK  0x3FFF
/** @brief MSI delivery mode bit (0=DMSI, 1=MMSI) */
#define APLIC_TARGET_MSI_DEL    BIT(11)
/** @brief External Interrupt Identity field mask (11-bit, bits 10:0) */
#define APLIC_TARGET_EIID_MASK  0x7FF
/** @} */

/**
 * @name APLIC GENMSI Register Fields
 * Bit field definitions for software-triggered MSI generation.
 * @{
 */
/** @brief Hart index field shift in GENMSI register */
#define APLIC_GENMSI_HART_SHIFT    18
/** @brief Hart index field mask (14-bit, bits 31:18) */
#define APLIC_GENMSI_HART_MASK     0x3FFF
/** @brief Context/Guest field shift (bits 17:13) */
#define APLIC_GENMSI_CONTEXT_SHIFT 13
/** @brief Context field mask (5-bit, for DMSI) */
#define APLIC_GENMSI_CONTEXT_MASK  0x1F
/** @brief Busy bit (read-only status) */
#define APLIC_GENMSI_BUSY          BIT(12)
/** @brief MSI delivery mode (0=DMSI, 1=MMSI) */
#define APLIC_GENMSI_MMSI_MODE     BIT(11)
/** @brief External Interrupt Identity field mask (11-bit, bits 10:0) */
#define APLIC_GENMSI_EIID_MASK     0x7FF
/** @} */

/**
 * @brief Calculate sourcecfg register offset for a source
 *
 * @param src Interrupt source number (1-based)
 * @return Register offset for the source's configuration register
 */
static inline uint32_t aplic_sourcecfg_off(unsigned int src)
{
	return APLIC_SOURCECFG_BASE + (src - 1U) * 4U;
}

/**
 * @brief Calculate target register offset for a source
 *
 * @param src Interrupt source number (1-based)
 * @return Register offset for the source's target register
 */
static inline uint32_t aplic_target_off(unsigned int src)
{
	return APLIC_TARGET_BASE + (src - 1U) * 4U;
}

/**
 * @brief Get the APLIC device instance
 *
 * Returns the device structure for the APLIC driver.
 *
 * @return Pointer to the APLIC device, or NULL if not available
 */
const struct device *riscv_aplic_get_dev(void);

/**
 * @brief Enable or disable the APLIC domain
 *
 * Controls the interrupt enable bit in the domain configuration register.
 *
 * @param dev APLIC device
 * @param enable true to enable the domain, false to disable
 * @return 0 on success, negative error code on failure
 */
int riscv_aplic_domain_enable(const struct device *dev, bool enable);

/**
 * @brief Configure an interrupt source mode
 *
 * Sets the trigger mode for an interrupt source (edge/level, polarity).
 *
 * @param dev APLIC device
 * @param src Interrupt source number
 * @param sm Source mode (APLIC_SM_* values)
 * @return 0 on success, negative error code on failure
 */
int riscv_aplic_config_src(const struct device *dev, unsigned int src, unsigned int sm);

/**
 * @brief Configure MSI routing for an interrupt source
 *
 * Sets the target hart and EIID for MSI delivery of the specified source.
 *
 * @param dev APLIC device
 * @param src Interrupt source number
 * @param hart Target hart index
 * @param eiid External Interrupt Identity for the target IMSIC
 * @return 0 on success, negative error code on failure
 */
int riscv_aplic_msi_route(const struct device *dev, unsigned int src, uint32_t hart, uint32_t eiid);

/**
 * @brief Enable or disable an interrupt source
 *
 * Controls whether the specified interrupt source can generate interrupts.
 *
 * @param dev APLIC device
 * @param src Interrupt source number
 * @param enable true to enable, false to disable
 * @return 0 on success, negative error code on failure
 */
int riscv_aplic_enable_src(const struct device *dev, unsigned int src, bool enable);

/**
 * @brief Inject a software-triggered MSI
 *
 * Triggers an MSI to the specified hart using the GENMSI register.
 *
 * @param dev APLIC device
 * @param eiid External Interrupt Identity
 * @param hart_id Target hart index
 * @param context Context field (for DMSI mode)
 * @return 0 on success, negative error code on failure
 */
int riscv_aplic_msi_inject_software_interrupt(const struct device *dev, uint32_t eiid,
					      uint32_t hart_id, uint32_t context);

/**
 * @brief Get the number of interrupt sources
 *
 * Returns the number of interrupt sources supported by the APLIC.
 *
 * @param dev APLIC device
 * @return Number of interrupt sources
 */
uint32_t riscv_aplic_get_num_sources(const struct device *dev);

/**
 * @brief Enable an interrupt source (convenience wrapper)
 *
 * Enables the specified interrupt source using the default APLIC device.
 *
 * @param src Interrupt source number
 */
static inline void riscv_aplic_enable_source(unsigned int src)
{
	const struct device *dev = riscv_aplic_get_dev();

	if (dev) {
		riscv_aplic_enable_src(dev, src, true);
	}
}

/**
 * @brief Disable an interrupt source (convenience wrapper)
 *
 * Disables the specified interrupt source using the default APLIC device.
 *
 * @param src Interrupt source number
 */
static inline void riscv_aplic_disable_source(unsigned int src)
{
	const struct device *dev = riscv_aplic_get_dev();

	if (dev) {
		riscv_aplic_enable_src(dev, src, false);
	}
}

/**
 * @brief Inject MSI using GENMSI (convenience wrapper)
 *
 * Injects a software MSI to the specified hart using the default APLIC device.
 *
 * @param hart Target hart index
 * @param eiid External Interrupt Identity
 */
static inline void riscv_aplic_msi_inject_genmsi(uint32_t hart, uint32_t eiid)
{
	const struct device *dev = riscv_aplic_get_dev();

	if (dev) {
		riscv_aplic_msi_inject_software_interrupt(dev, eiid, hart, 0);
	}
}

#endif
