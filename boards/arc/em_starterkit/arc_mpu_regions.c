/*
 * Copyright (c) 2017 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <soc.h>
#include <zephyr/arch/arc/v2/mpu/arc_mpu.h>
#include <zephyr/linker/linker-defs.h>

/*
 * for secure firmware, MPU entries are only set up for secure world.
 * All regions not listed here are shared by secure world and normal world.
 */
static struct arc_mpu_region mpu_regions[] = {
#if DT_REG_SIZE(DT_INST(0, arc_iccm)) > 0
	/* Region ICCM */
	MPU_REGION_ENTRY("ICCM",
			 DT_REG_ADDR(DT_INST(0, arc_iccm)),
			 DT_REG_SIZE(DT_INST(0, arc_iccm)),
			 REGION_ROM_ATTR),
#endif
#if DT_REG_SIZE(DT_INST(0, arc_dccm)) > 0
	/* Region DCCM */
	MPU_REGION_ENTRY("DCCM",
			 DT_REG_ADDR(DT_INST(0, arc_dccm)),
			 DT_REG_SIZE(DT_INST(0, arc_dccm)),
			 REGION_KERNEL_RAM_ATTR | REGION_DYNAMIC),
#endif

#if DT_REG_SIZE(DT_INST(0, mmio_sram)) > 0
	/* Region DDR RAM */
	MPU_REGION_ENTRY("DDR RAM",
			DT_REG_ADDR(DT_INST(0, mmio_sram)),
			DT_REG_SIZE(DT_INST(0, mmio_sram)),
			AUX_MPU_ATTR_KW | AUX_MPU_ATTR_KR | AUX_MPU_ATTR_UR |
			AUX_MPU_ATTR_KE | AUX_MPU_ATTR_UE | REGION_DYNAMIC),
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
			 REGION_KERNEL_RAM_ATTR)
#endif
};

struct arc_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
