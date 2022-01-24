/*
 * Copyright (c) 2021 Weidmueller Interface GmbH & Co. KG
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <devicetree.h>
#include <init.h>
#include <sys/util.h>

#include <arch/arm/aarch32/mmu/arm_mmu.h>
#include "soc.h"

static const struct arm_mmu_region mmu_regions[] = {

	MMU_REGION_FLAT_ENTRY("vectors",
			      0x00000000,
			      0x1000,
			      MT_STRONGLY_ORDERED | MPERM_R | MPERM_X),
	MMU_REGION_FLAT_ENTRY("slcr",
			      0xF8000000,
			      0x1000,
			      MT_STRONGLY_ORDERED | MPERM_R | MPERM_W),
	MMU_REGION_FLAT_ENTRY("mpcore",
			      0xF8F00000,
			      0x2000,
			      MT_STRONGLY_ORDERED | MPERM_R | MPERM_W),
	MMU_REGION_FLAT_ENTRY("ocm",
			      DT_REG_ADDR(DT_CHOSEN(zephyr_ocm)),
			      DT_REG_SIZE(DT_CHOSEN(zephyr_ocm)),
			      MT_STRONGLY_ORDERED | MPERM_R | MPERM_W),
	/* ARM Arch timer, GIC are covered by the MPCore mapping */
/* UARTs */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart0), okay)
	MMU_REGION_FLAT_ENTRY("uart0",
			      DT_REG_ADDR(DT_NODELABEL(uart0)),
			      DT_REG_SIZE(DT_NODELABEL(uart0)),
			      MT_DEVICE | MATTR_SHARED | MPERM_R | MPERM_W),
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay)
	MMU_REGION_FLAT_ENTRY("uart1",
			      DT_REG_ADDR(DT_NODELABEL(uart1)),
			      DT_REG_SIZE(DT_NODELABEL(uart1)),
			      MT_DEVICE | MATTR_SHARED | MPERM_R | MPERM_W),
#endif

/* GEMs */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gem0), okay)
	MMU_REGION_FLAT_ENTRY("gem0",
			      DT_REG_ADDR(DT_NODELABEL(gem0)),
			      DT_REG_SIZE(DT_NODELABEL(gem0)),
			      MT_DEVICE | MATTR_SHARED | MPERM_R | MPERM_W),
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gem1), okay)
	MMU_REGION_FLAT_ENTRY("gem1",
			      DT_REG_ADDR(DT_NODELABEL(gem1)),
			      DT_REG_SIZE(DT_NODELABEL(gem1)),
			      MT_DEVICE | MATTR_SHARED | MPERM_R | MPERM_W),
#endif
};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	.mmu_regions = mmu_regions,
};

/**
 * @brief Basic hardware initialization of the Zynq-7000 SoC
 *
 * Performs the basic initialization of the Zynq-7000 SoC.
 *
 * @return 0
 */
static int soc_xlnx_zynq7000_init(const struct device *arg)
{
	ARG_UNUSED(arg);
	NMI_INIT();

	return 0;
}

SYS_INIT(soc_xlnx_zynq7000_init, PRE_KERNEL_1,
	CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

/* EOF */
