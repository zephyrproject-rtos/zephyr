/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/arch/arm/mpu/arm_mpu_mem_cfg.h>

#ifdef CONFIG_ARM_MPU_SRAM_WRITE_THROUGH
#define ARM_MPU_RAM_REGION_ATTR REGION_RAM_WT_ATTR
#else
#define ARM_MPU_RAM_REGION_ATTR REGION_RAM_ATTR
#endif

#define MEMORY_REGION_SIZE_KB(SIZE)    (SIZE / 1024)

#define OCRAM1_SHM_SIZE                 DT_REG_SIZE_BY_IDX(DT_NODELABEL(ocram1), 0)
#define HYPER_RAM_SIZE                  DT_REG_SIZE_BY_IDX(DT_NODELABEL(flexspi), 1)

#define REGION_OCRAM1_SHM_BASE_ADDRESS   DT_REG_ADDR_BY_IDX(DT_NODELABEL(ocram1), 0)
#define REGION_OCRAM1_SHM_SIZE           \
			REGION_CUSTOMED_MEMORY_SIZE(MEMORY_REGION_SIZE_KB(OCRAM1_SHM_SIZE))
#define REGION_HYPER_RAM_BASE_ADDRESS DT_REG_ADDR_BY_IDX(DT_NODELABEL(flexspi), 1)
#define REGION_HYPER_RAM_SIZE            \
			REGION_CUSTOMED_MEMORY_SIZE(MEMORY_REGION_SIZE_KB(HYPER_RAM_SIZE))

/*
 * Guards: when a RAM node is selected as zephyr,flash the FLASH_0 entry
 * already maps it RO/X.  Adding a second RW/XN RAM entry would silently
 * downgrade the region to data-only at the higher MPU index.
 */
#define OCRAM1_IS_CHOSEN_FLASH                                                                     \
	DT_NODE_EXISTS(DT_CHOSEN(zephyr_flash)) &&                                                 \
		DT_SAME_NODE(DT_CHOSEN(zephyr_flash), DT_NODELABEL(ocram1))

#define HYPERRAM0_IS_CHOSEN_FLASH                                                                  \
	DT_NODE_EXISTS(DT_CHOSEN(zephyr_flash)) &&                                                 \
		DT_SAME_NODE(DT_CHOSEN(zephyr_flash), DT_NODELABEL(hyperram0))

static const struct arm_mpu_region mpu_regions[] = {
/*
 * The catch-all no-access region for unmapped addresses (Arm
 * Cortex-M7 erratum 1013783) is programmed by the MPU driver,
 * see CONFIG_ARM_MPU_CM7_UNMAPPED_REGION.
 */

#ifdef CONFIG_XIP
	MPU_REGION_ENTRY("FLASH_0", CONFIG_FLASH_BASE_ADDRESS,
#if defined(CONFIG_ARMV8_M_BASELINE) || defined(CONFIG_ARMV8_M_MAINLINE)
			 REGION_FLASH_ATTR(CONFIG_FLASH_BASE_ADDRESS, CONFIG_FLASH_SIZE * 1024)),
#else
			 REGION_FLASH_ATTR(REGION_FLASH_SIZE)),
#endif
#endif

	MPU_REGION_ENTRY("SRAM_0", DT_CHOSEN_SRAM_ADDR,
#if defined(CONFIG_ARMV8_M_BASELINE) || defined(CONFIG_ARMV8_M_MAINLINE)
			 ARM_MPU_RAM_REGION_ATTR(DT_CHOSEN_SRAM_ADDR, DT_CHOSEN_SRAM_SIZE)),
#else
			 ARM_MPU_RAM_REGION_ATTR(REGION_SRAM_SIZE)),
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ocram1)) && !(OCRAM1_IS_CHOSEN_FLASH)
	MPU_REGION_ENTRY("OCRAM1", REGION_OCRAM1_SHM_BASE_ADDRESS,
			 ARM_MPU_RAM_REGION_ATTR(REGION_OCRAM1_SHM_SIZE)),
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(hyperram0)) && !(HYPERRAM0_IS_CHOSEN_FLASH)
	MPU_REGION_ENTRY("HYPER_RAM", REGION_HYPER_RAM_BASE_ADDRESS,
			 ARM_MPU_RAM_REGION_ATTR(REGION_HYPER_RAM_SIZE)),
#endif
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
