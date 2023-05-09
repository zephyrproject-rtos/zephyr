/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>

#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/drivers/pcie/cap.h>

#include "vc.h"

uint32_t pcie_vc_cap_lookup(pcie_bdf_t bdf, struct pcie_vc_regs *regs)
{
	uint32_t base;

	base = pcie_get_ext_cap(bdf, PCIE_EXT_CAP_ID_VC);
	if (base == 0) {
		base = pcie_get_ext_cap(bdf, PCIE_EXT_CAP_ID_MFVC_VC);
		if (base == 0) {
			return 0;
		}
	}

	regs->cap_reg_1.raw = pcie_conf_read(bdf, base +
					     PCIE_VC_CAP_REG_1_OFFSET);
	regs->cap_reg_2.raw = pcie_conf_read(bdf, base +
					     PCIE_VC_CAP_REG_2_OFFSET);
	regs->ctrl_reg.raw = pcie_conf_read(bdf, base +
					    PCIE_VC_CTRL_STATUS_REG_OFFSET);

	return base;
}

void pcie_vc_load_resources_regs(pcie_bdf_t bdf,
				 uint32_t base,
				 struct pcie_vc_resource_regs *regs,
				 int nb_regs)
{
	int idx;

	for (idx = 0; idx < nb_regs; idx++) {
		regs->cap_reg.raw =
			pcie_conf_read(bdf, base +
				       PCIE_VC_RES_CAP_REG_OFFSET(idx));
		regs->ctrl_reg.raw =
			pcie_conf_read(bdf, base +
				       PCIE_VC_RES_CTRL_REG_OFFSET(idx));
		regs->status_reg.raw =
			pcie_conf_read(bdf, base +
				       PCIE_VC_RES_STATUS_REG_OFFSET(idx));
		regs++;
	}
}
