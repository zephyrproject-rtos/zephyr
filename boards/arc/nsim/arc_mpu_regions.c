/*
 * Copyright (c) 2017 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <generated_dts_board.h>
#include <soc.h>
#include <arch/arc/v2/mpu/arc_mpu.h>
#include <linker/linker-defs.h>

/*
 * for secure firmware, MPU entries are only set up for secure world.
 * All regions not listed here are shared by secure world and normal world.
 */
static struct arc_mpu_region mpu_regions[] = {
#if DT_ICCM_SIZE > 0
	/* Region ICCM */
	MPU_REGION_ENTRY("ICCM",
			 DT_ICCM_BASE_ADDRESS,
			 DT_ICCM_SIZE * 1024,
			 REGION_ROM_ATTR),
#endif
#if DT_DCCM_SIZE > 0
	/* Region DCCM */
	MPU_REGION_ENTRY("DCCM",
			 DT_DCCM_BASE_ADDRESS,
			 DT_DCCM_SIZE * 1024,
			 REGION_KERNEL_RAM_ATTR | REGION_DYNAMIC),
#endif

/*
 * Region peripheral is shared by secure world and normal world by default,
 * no need a static mpu entry. If some peripherals belong to secure world,
 * add it here.
 */
#ifndef CONFIG_ARC_SECURE_FIRMWARE
	/* Region Peripheral */
	MPU_REGION_ENTRY("PERIPHERAL",
			 0xF0000000,
			 64 * 1024,
			 REGION_KERNEL_RAM_ATTR),
#endif
};

struct arc_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
