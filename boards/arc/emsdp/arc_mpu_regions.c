/*
 * Copyright (c) 2019 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/arch/arc/v2/mpu/arc_mpu.h>
#include <zephyr/linker/linker-defs.h>

static struct arc_mpu_region mpu_regions[] = {
	/* Region ICCM */
	MPU_REGION_ENTRY("ICCM",
			 DT_REG_ADDR(DT_INST(0, arc_iccm)),
			 DT_REG_SIZE(DT_INST(0, arc_iccm)),
			 REGION_ROM_ATTR),
	/* Region DCCM */
	MPU_REGION_ENTRY("DCCM",
			 DT_REG_ADDR(DT_INST(0, arc_dccm)),
			 DT_REG_SIZE(DT_INST(0, arc_dccm)),
			 REGION_KERNEL_RAM_ATTR | REGION_DYNAMIC),
	/* Region XCCM */
#if DT_REG_SIZE(DT_INST(0, arc_xccm)) > 0
	MPU_REGION_ENTRY("XCCM",
			 DT_REG_ADDR(DT_INST(0, arc_xccm)),
			 DT_REG_SIZE(DT_INST(0, arc_xccm)),
			 REGION_KERNEL_RAM_ATTR | REGION_DYNAMIC),
#endif
	/* Region YCCM */
#if DT_REG_SIZE(DT_INST(0, arc_yccm)) > 0
	MPU_REGION_ENTRY("YCCM",
			 DT_REG_ADDR(DT_INST(0, arc_yccm)),
			 DT_REG_SIZE(DT_INST(0, arc_yccm)),
			 REGION_KERNEL_RAM_ATTR | REGION_DYNAMIC),
#endif
	/* Region DDR RAM */
	MPU_REGION_ENTRY("SRAM",
			DT_REG_ADDR(DT_INST(0, mmio_sram)),
			DT_REG_SIZE(DT_INST(0, mmio_sram)),
			REGION_KERNEL_RAM_ATTR |
			AUX_MPU_ATTR_KW | AUX_MPU_ATTR_KR | AUX_MPU_ATTR_UR |
			AUX_MPU_ATTR_KE | AUX_MPU_ATTR_UE | REGION_DYNAMIC),
	/* Region Peripheral */
	MPU_REGION_ENTRY("PERIPHERAL",
			 0xF0000000,
			 64 * 1024,
			 REGION_KERNEL_RAM_ATTR),
};

struct arc_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
