/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PCIE_MSI_H_
#define ZEPHYR_INCLUDE_DRIVERS_PCIE_MSI_H_

#include <drivers/pcie/pcie.h>
#include <zephyr/types.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Find a PCI(e) capability in an endpoint's configuration space.
 *
 * @param bdf the PCI endpoint to examine
 * @param cap_id the capability ID of interest
 * @return the index of the configuration word, or 0 if no capability.
 *
 * Note: PCI(e) capabilities are only used in the MSI code, so for
 * now, capabilities-related code is only included when MSI is. It
 * can easily be separated out if/when its use spreads.
 */
extern uint32_t pcie_get_cap(pcie_bdf_t bdf, uint32_t cap_id);

/*
 * Configuration word 13 contains the head of the capabilities list.
 */

#define PCIE_CONF_CAPPTR	13U	/* capabilities pointer */
#define PCIE_CONF_CAPPTR_FIRST(w)	(((w) >> 2) & 0x3FU)

/*
 * The first word of every capability contains a capability identifier,
 * and a link to the next capability (or 0) in configuration space.
 */

#define PCIE_CONF_CAP_ID(w)		((w) & 0xFFU)
#define PCIE_CONF_CAP_NEXT(w)		(((w) >> 10) & 0x3FU)

/**
 * @brief Compute the target address for an MSI posted write.
 *
 * This function is exported by the arch, board or SoC code.
 *
 * @param irq The IRQ we wish to trigger via MSI.
 * @return A (32-bit) value for the MSI MAP register.
 */
extern uint32_t pcie_msi_map(unsigned int irq);

/**
 * @brief Compute the data for an MSI posted write.
 *
 * This function is exported by the arch, board or SoC code.
 *
 * @param irq The IRQ we wish to trigger via MSI.
 * @return A (16-bit) value for MSI MDR register.
 */
extern uint16_t pcie_msi_mdr(unsigned int irq);

/**
 * @brief Configure the given PCI endpoint to generate MSIs.
 *
 * @param bdf the target PCI endpoint
 * @param irq the IRQ which should be generated
 * @return true if the endpoint supports MSI, false otherwise.
 */
extern bool pcie_set_msi(pcie_bdf_t bdf, unsigned int irq);

/*
 * MSI capability IDs in the PCI configuration capability list.
 */

#define PCIE_MSI_CAP_ID		0x05U
#define PCIE_MSIX_CAP_ID	0x11U

/*
 * The first word of the MSI capability is shared with the
 * capability ID and list link.  The high 16 bits are the MCR.
 */

#define PCIE_MSI_MCR		0U

#define PCIE_MSI_MCR_EN		0x00010000U  /* enable MSI */
#define PCIE_MSI_MCR_MME	0x00700000U  /* mask of # of enabled IRQs */
#define PCIE_MSI_MCR_64		0x00800000U  /* 64-bit MSI */

/*
 * The MAP follows the MCR. If PCIE_MSI_MCR_64, then the MAP
 * is two words long. The MDR follows immediately after the MAP.
 */

#define PCIE_MSI_MAP0		1U
#define PCIE_MSI_MAP1_64	2U
#define PCIE_MSI_MDR_32		2U
#define PCIE_MSI_MDR_64		3U

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_PCIE_MSI_H_ */
