/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_PCIE_HOST_VC_H_
#define ZEPHYR_DRIVERS_PCIE_HOST_VC_H_

#include <zephyr/sys/util.h>

/* pcie_conf_read()/pcie_conf_write() use DWORD indices, not byte offsets. */
#define PCIE_VC_REG_OFFSET(_offset)		((_offset) / sizeof(uint32_t))

#define PCIE_VC_CAP_REG_1_OFFSET	PCIE_VC_REG_OFFSET(0x04U)
#define PCIE_VC_CAP_REG_2_OFFSET	PCIE_VC_REG_OFFSET(0x08U)
#define PCIE_VC_CTRL_STATUS_REG_OFFSET	PCIE_VC_REG_OFFSET(0x0CU)

#define PCIE_VC_CAP1_EVCC_MASK			GENMASK(2, 0)
#define PCIE_VC_CAP1_LPEVCC_MASK		GENMASK(6, 4)
#define PCIE_VC_CAP1_PAT_ENTRY_SIZE_MASK	GENMASK(12, 10)
#define PCIE_VC_CAP2_VCA_CAP_MASK		GENMASK(7, 0)
#define PCIE_VC_CAP2_VCA_TABLE_OFFSET_MASK	GENMASK(31, 24)

/** Virtual Channel capability and control registers. */
struct pcie_vc_regs {
	uint32_t cap_reg_1;
	uint32_t cap_reg_2;
	uint32_t ctrl_reg;
};

#define PCIE_VC_RES_CAP_REG_OFFSET(_vc) \
	PCIE_VC_REG_OFFSET(0x10U + (_vc) * 0x0CU)
#define PCIE_VC_RES_CTRL_REG_OFFSET(_vc) \
	PCIE_VC_REG_OFFSET(0x14U + (_vc) * 0x0CU)
#define PCIE_VC_RES_STATUS_REG_OFFSET(_vc) \
	PCIE_VC_REG_OFFSET(0x18U + (_vc) * 0x0CU)

#define PCIE_VC_PA_RR		BIT(0)
#define PCIE_VC_PA_WRR		BIT(1)
#define PCIE_VC_PA_WRR64	BIT(2)
#define PCIE_VC_PA_WRR128	BIT(3)
#define PCIE_VC_PA_TMWRR128	BIT(4)
#define PCIE_VC_PA_WRR256	BIT(5)

#define PCIE_VC_RES_CAP_PA_CAP_MASK		GENMASK(7, 0)
#define PCIE_VC_RES_CAP_RST			BIT(15)
#define PCIE_VC_RES_CAP_MAX_TIME_SLOTS_MASK	GENMASK(22, 16)
#define PCIE_VC_RES_CAP_PA_TABLE_OFFSET_MASK	GENMASK(31, 24)
#define PCIE_VC_RES_CTRL_TC_MAP_MASK		GENMASK(7, 0)
#define PCIE_VC_RES_CTRL_PA_SELECT_MASK		GENMASK(19, 17)
#define PCIE_VC_RES_CTRL_VC_ID_MASK		GENMASK(26, 24)
#define PCIE_VC_RES_CTRL_ENABLE			BIT(31)
#define PCIE_VC_RES_STATUS_NEGOTIATION_PENDING	BIT(17)

/** Virtual Channel Resource registers. */
struct pcie_vc_resource_regs {
	uint32_t cap_reg;
	uint32_t ctrl_reg;
	uint32_t status_reg;
};

uint32_t pcie_vc_cap_lookup(pcie_bdf_t bdf, struct pcie_vc_regs *regs);

void pcie_vc_load_resources_regs(pcie_bdf_t bdf,
				 uint32_t base,
				 struct pcie_vc_resource_regs *regs,
				 int nb_regs);

#endif /* ZEPHYR_DRIVERS_PCIE_HOST_VC_H_ */
