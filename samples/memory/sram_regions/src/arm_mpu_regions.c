/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/slist.h>
#include <arch/arm/aarch32/mpu/arm_mpu.h>
#include <linker/devicetree_sram.h>

#define BUILD_MPU_REGION(p_name, p_base, p_size, p_attr)	\
	{ .name = p_name,					\
	  .base = p_base,					\
	  .attr.rasr = p_attr,					\
	},

static const struct arm_mpu_region mpu_regions[] = {
	/* FLASH */
	MPU_REGION_ENTRY("FLASH_0",
			 CONFIG_FLASH_BASE_ADDRESS,
			 REGION_FLASH_ATTR(REGION_4M)),

	/* SRAM0 */
	MPU_REGION_ENTRY("SRAM_0",
			 CONFIG_SRAM_BASE_ADDRESS,
			 REGION_RAM_ATTR(REGION_2M)),

	/* Generated SRAM regions */
	DT_SRAM_INST_FOREACH(BUILD_MPU_REGION)
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
