/*
 * Copyright (c) 2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/arch/arm/mpu/arm_mpu_mem_cfg.h>

/*
 * Define noncacheable flash region attributes using noncacheable SRAM memory
 * attribute index.
 */
#define MAX32_FLASH_NON_CACHEABLE(base, size)                                                      \
	{                                                                                          \
		.rbar = RO_Msk | NON_SHAREABLE_Msk,                                                \
		.mair_idx = MPU_MAIR_INDEX_SRAM_NOCACHE,                                           \
		.r_limit = REGION_LIMIT_ADDR(base, size),                                          \
	}

#define MAX32_MPU_REGION(name, base, attr, size) MPU_REGION_ENTRY(name, (base), attr((base), size))

#ifdef CONFIG_SRAM_DEPRECATED_KCONFIG_SET
#define RAM_ADDR CONFIG_SRAM_BASE_ADDRESS
#define RAM_SIZE KB(CONFIG_SRAM_SIZE)
#else
#define RAM_ADDR DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
#define RAM_SIZE DT_REG_SIZE(DT_CHOSEN(zephyr_sram))
#endif

/*
 * The MPU regions are defined in the following way:
 * - Cacheable flash region
 * - Non-cacheable flash region, i.e., storage area at the end of the flash
 * - SRAM region
 * If the storage partition is not defined, the flash region spans the whole
 * flash.
 */
static const struct arm_mpu_region mpu_regions[] = {
#if PARTITION_EXISTS(storage_partition)
#define STORAGE_ADDR (CONFIG_FLASH_BASE_ADDRESS + PARTITION_OFFSET(storage_partition))
#define STORAGE_SIZE (PARTITION_SIZE(storage_partition) >> 10)
	MAX32_MPU_REGION("FLASH", CONFIG_FLASH_BASE_ADDRESS, REGION_FLASH_ATTR,
			 KB(CONFIG_FLASH_SIZE - STORAGE_SIZE)),
	MAX32_MPU_REGION("STORAGE", STORAGE_ADDR, MAX32_FLASH_NON_CACHEABLE, KB(STORAGE_SIZE)),
#else
	MAX32_MPU_REGION("FLASH", CONFIG_FLASH_BASE_ADDRESS, REGION_FLASH_ATTR,
			 KB(CONFIG_FLASH_SIZE)),
#endif
	MAX32_MPU_REGION("SRAM", RAM_ADDR, REGION_RAM_ATTR, RAM_SIZE),
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
