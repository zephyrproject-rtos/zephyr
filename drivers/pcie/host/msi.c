/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <drivers/pcie/pcie.h>
#include <drivers/pcie/msi.h>

/* functions documented in include/drivers/pcie/msi.h */

bool pcie_set_msi(pcie_bdf_t bdf, unsigned int irq)
{
	bool success = false; /* keepin' the MISRA peeps employed */
	uint32_t base;
	uint32_t mcr;
	uint32_t map;
	uint32_t mdr;

	map = pcie_msi_map(irq);
	mdr = pcie_msi_mdr(irq);
	base = pcie_get_cap(bdf, PCIE_MSI_CAP_ID);

	if (base != 0U) {
		mcr = pcie_conf_read(bdf, base + PCIE_MSI_MCR);
		pcie_conf_write(bdf, base + PCIE_MSI_MAP0, map);

		if (mcr & PCIE_MSI_MCR_64) {
			pcie_conf_write(bdf, base + PCIE_MSI_MAP1_64, 0U);
			pcie_conf_write(bdf, base + PCIE_MSI_MDR_64, mdr);
		} else {
			pcie_conf_write(bdf, base + PCIE_MSI_MDR_32, mdr);
		}

		mcr |= PCIE_MSI_MCR_EN;
		mcr &= ~PCIE_MSI_MCR_MME;  /* only 1 IRQ please */
		pcie_conf_write(bdf, base + PCIE_MSI_MCR, mcr);
		pcie_set_cmd(bdf, PCIE_CONF_CMDSTAT_MASTER, true);
		success = true;
	}

	return success;
}
