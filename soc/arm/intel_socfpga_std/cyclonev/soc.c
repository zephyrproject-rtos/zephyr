/*
 * Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 * Description:
 * Adding support for Cyclone V SoC FPGA, buildi mmu table,
 * creating function to reserve the vector memory area
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>
#include <mmu.h>
#include <zephyr/arch/arm/aarch32/mmu/arm_mmu.h>
#include <zephyr/arch/arm/aarch32/cortex_a_r/cmsis.h>
#include <zephyr/arch/arm/aarch32/nmi.h>
#include "soc.h"

void arch_reserved_pages_update(void)
{
/* Function created to reserve the vector table  */
	uintptr_t addr = 0;
	size_t len = 0x1000;

	uintptr_t pos = ROUND_DOWN(addr, CONFIG_MMU_PAGE_SIZE);
	uintptr_t end = ROUND_UP(addr + len, CONFIG_MMU_PAGE_SIZE);

	for (; pos < end; pos += CONFIG_MMU_PAGE_SIZE) {
		if (!z_is_page_frame(pos)) {
			continue;
		}
		struct z_page_frame *pf = z_phys_to_page_frame(pos);

		pf->flags |= Z_PAGE_FRAME_RESERVED;
	}
}

static const struct arm_mmu_region mmu_regions[] = {

	MMU_REGION_FLAT_ENTRY("vectors",
		0x00000000,
		0x1000,
		MT_STRONGLY_ORDERED | MPERM_R | MPERM_X),

	MMU_REGION_FLAT_ENTRY("mpcore",
		0xF8F00000,
		0x2000,
		MT_STRONGLY_ORDERED | MPERM_R | MPERM_W),

	MMU_REGION_FLAT_ENTRY("devices",
		0xFF000000,
		0x1000000,
		MT_DEVICE | MATTR_SHARED | MPERM_R | MPERM_W
/* Adding Executable property to devices when using for testing */
#ifndef CONFIG_ASSERT
		| MPERM_X
#endif
		),
};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	 .mmu_regions = mmu_regions,
};

/**
 * @brief Basic hardware initialization of the Cyclone V SoC FPGA
 *
 * Performs the basic initialization of the Cyclone V SoC FPGA.
 *
 * @return 0
 */
static int soc_intel_cyclonev_init(void)
{
	NMI_INIT();
	unsigned int sctlr = __get_SCTLR(); /* modifying some registers prior to initialization */

	sctlr &= ~SCTLR_A_Msk;
	__set_SCTLR(sctlr);
	__set_VBAR(0);
	return 0;
}

SYS_INIT(soc_intel_cyclonev_init, PRE_KERNEL_1,
	CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
/* EOF */
