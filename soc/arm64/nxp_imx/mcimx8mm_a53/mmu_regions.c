/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <arch/arm64/arm_mmu.h>

static const struct arm_mmu_region mmu_regions[] = {

	MMU_REGION_FLAT_ENTRY("GIC",
			      DT_REG_ADDR_BY_IDX(DT_INST(0, arm_gic), 0),
			      DT_REG_SIZE_BY_IDX(DT_INST(0, arm_gic), 0),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_RW | MT_NS),

	MMU_REGION_FLAT_ENTRY("GIC",
			      DT_REG_ADDR_BY_IDX(DT_INST(0, arm_gic), 1),
			      DT_REG_SIZE_BY_IDX(DT_INST(0, arm_gic), 1),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_RW | MT_NS),

	MMU_REGION_FLAT_ENTRY("UART",
			      DT_REG_ADDR(DT_INST(0, nxp_imx_iuart)),
			      DT_REG_SIZE(DT_INST(0, nxp_imx_iuart)),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_RW | MT_NS),

	MMU_REGION_FLAT_ENTRY("ANA_PLL",
			      DT_REG_ADDR(DT_INST(0, nxp_imx_ana)),
			      DT_REG_SIZE(DT_INST(0, nxp_imx_ana)),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_RW | MT_NS),

	MMU_REGION_FLAT_ENTRY("UART",
			      DT_REG_ADDR(DT_INST(1, nxp_imx_iuart)),
			      DT_REG_SIZE(DT_INST(1, nxp_imx_iuart)),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_RW | MT_NS),
	/* TODO: Add CCM */

};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	.mmu_regions = mmu_regions,
};
