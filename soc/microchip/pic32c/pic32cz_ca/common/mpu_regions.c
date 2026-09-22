/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>
#include <zephyr/arch/arm/mpu/arm_mpu_mem_cfg.h>

#define FLASH_NODE DT_CHOSEN(zephyr_flash)
#define SRAM_NODE  DT_CHOSEN(zephyr_sram)

#define BFM_BASE DT_REG_ADDR(FLASH_NODE)
#define BFM_SIZE (DT_PROP_BY_IDX(FLASH_NODE, unimplemented_region, 0) - BFM_BASE)

#define PFM_BASE (DT_PROP_BY_IDX(FLASH_NODE, unimplemented_region, 1) + 1)
#define PFM_SIZE (DT_REG_ADDR(FLASH_NODE) + DT_REG_SIZE(FLASH_NODE) - PFM_BASE)

#define RAM_BASE       0x20000000
#define RAM_SPAN       (DT_REG_ADDR(SRAM_NODE) + DT_REG_SIZE(SRAM_NODE) - RAM_BASE)
#define RAM_REGION     BIT(LOG2CEIL(RAM_SPAN))
#define RAM_SUBREGIONS DIV_ROUND_UP(RAM_SPAN, RAM_REGION / 8)
#define RAM_SRD        (((0xFFU << RAM_SUBREGIONS) & 0xFFU) << MPU_RASR_SRD_Pos)

#if defined(CONFIG_ARM_MPU_SRAM_WRITE_THROUGH)
#define RAM_TYPE NORMAL_OUTER_INNER_WRITE_THROUGH_NON_SHAREABLE
#else
#define RAM_TYPE NORMAL_OUTER_INNER_WRITE_BACK_WRITE_READ_ALLOCATE_NON_SHAREABLE
#endif

#define PERIPH_BASE 0x44000000

static const struct arm_mpu_region mpu_regions[] = {
	MPU_REGION_ENTRY("FLASH_BFM", BFM_BASE,
			 REGION_FLASH_ATTR(REGION_CUSTOMED_MEMORY_SIZE(BFM_SIZE / 1024))),
	MPU_REGION_ENTRY("FLASH_PFM", PFM_BASE,
			 REGION_FLASH_ATTR(REGION_CUSTOMED_MEMORY_SIZE(PFM_SIZE / 1024))),
	MPU_REGION_ENTRY("SRAM_0", RAM_BASE,
			 {RAM_TYPE | MPU_RASR_XN_Msk | P_RW_U_NA_Msk | RAM_SRD |
			  REGION_CUSTOMED_MEMORY_SIZE(RAM_REGION / 1024)}),
	MPU_REGION_ENTRY("PERIPHERALS", PERIPH_BASE, REGION_IO_ATTR(REGION_64M)),
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
