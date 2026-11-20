/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm/mpu/arm_mpu.h>
#include <zephyr/arch/arm/mpu/arm_mpu_mem_cfg.h>
#include <zephyr/devicetree.h>
#include <zephyr/devicetree/partitions.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_ARM_MPU_SRAM_WRITE_THROUGH
#define ALIF_SRAM_REGION_ATTR REGION_RAM_WT_ATTR
#else
#define ALIF_SRAM_REGION_ATTR REGION_RAM_ATTR
#endif

/*
 * MRAM is written through Device memory, which cannot be executed. Flash is executable up to the
 * end of the code partition, or of CONFIG_FLASH_SIZE, and the rest of it up to mram_atoc is Device
 * memory. An MCUboot application also writes the image trailer at the end of its slot.
 */
#ifdef CONFIG_USE_DT_CODE_PARTITION
#define ALIF_CODE_END                                                                              \
	(DT_PARTITION_ADDR(DT_CHOSEN(zephyr_code_partition)) +                                     \
	 DT_REG_SIZE(DT_CHOSEN(zephyr_code_partition)))
#else
#define ALIF_CODE_END (CONFIG_FLASH_BASE_ADDRESS + CONFIG_FLASH_SIZE * 1024U)
#endif

#ifdef CONFIG_BOOTLOADER_MCUBOOT
#define ALIF_FLASH_END                                                                             \
	(ALIF_CODE_END - MAX(ROUND_UP(CONFIG_ROM_END_OFFSET, 32U),                                 \
			     DT_PROP(DT_CHOSEN(zephyr_flash), erase_block_size)))
#else
#define ALIF_FLASH_END ALIF_CODE_END
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(mram_atoc))
#define ALIF_DEVICE_END DT_REG_ADDR(DT_NODELABEL(mram_atoc))
#else
#define ALIF_DEVICE_END                                                                            \
	(DT_REG_ADDR(DT_CHOSEN(zephyr_flash)) + DT_REG_SIZE(DT_CHOSEN(zephyr_flash)))
#endif

BUILD_ASSERT((ALIF_FLASH_END % 32U) == 0U, "MPU regions have a 32-byte granule");

static const struct arm_mpu_region mpu_regions[] = {
	MPU_REGION_ENTRY("FLASH", CONFIG_FLASH_BASE_ADDRESS,
			 REGION_FLASH_ATTR(CONFIG_FLASH_BASE_ADDRESS,
					   ALIF_FLASH_END - CONFIG_FLASH_BASE_ADDRESS)),

	MPU_REGION_ENTRY("SRAM", DT_CHOSEN_SRAM_ADDR,
			 ALIF_SRAM_REGION_ATTR(DT_CHOSEN_SRAM_ADDR, DT_CHOSEN_SRAM_SIZE)),

#if defined(CONFIG_USE_DT_CODE_PARTITION) ||                                                       \
	((CONFIG_FLASH_BASE_ADDRESS + CONFIG_FLASH_SIZE * 1024U) < ALIF_DEVICE_END)
	MPU_REGION_ENTRY("MRAM_DEVICE", ALIF_FLASH_END,
			 REGION_DEVICE_ATTR(ALIF_FLASH_END, ALIF_DEVICE_END - ALIF_FLASH_END)),
#endif
};

#if defined(CONFIG_USE_DT_CODE_PARTITION)
BUILD_ASSERT(ALIF_FLASH_END < ALIF_DEVICE_END, "Code partition must end below the Device region");
#endif

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
