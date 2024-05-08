/*
 * Copyright (c) 2024 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/arch/arm/cortex_m/arm_mpu_mem_cfg.h>

#define USBHS_BASE	DT_REG_ADDR_BY_NAME(DT_NODELABEL(usbhs), core)
#define USBHS_SIZE	DT_REG_SIZE_BY_NAME(DT_NODELABEL(usbhs), core)

#define CAN120_BASE	DT_REG_ADDR_BY_NAME(DT_NODELABEL(can120), message_ram)
#define CAN120_SIZE	DT_REG_SIZE_BY_NAME(DT_NODELABEL(can120), message_ram) + \
			DT_REG_SIZE_BY_NAME(DT_NODELABEL(can120), m_can)

static struct arm_mpu_region mpu_regions[] = {
	MPU_REGION_ENTRY("FLASH_0",
			 CONFIG_FLASH_BASE_ADDRESS,
			 REGION_FLASH_ATTR(CONFIG_FLASH_BASE_ADDRESS,
				CONFIG_FLASH_SIZE * 1024)),
	MPU_REGION_ENTRY("SRAM_0",
			 CONFIG_SRAM_BASE_ADDRESS,
			 REGION_RAM_ATTR(CONFIG_SRAM_BASE_ADDRESS,
				CONFIG_SRAM_SIZE * 1024)),

#if DT_NODE_HAS_STATUS(DT_NODELABEL(usbhs), okay)
	MPU_REGION_ENTRY("USBHS_CORE", USBHS_BASE,
			 REGION_RAM_NOCACHE_ATTR(USBHS_BASE, USBHS_SIZE)),
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(can120), okay)
	MPU_REGION_ENTRY("CAN120_MCAN", CAN120_BASE,
			 REGION_RAM_NOCACHE_ATTR(CAN120_BASE, CAN120_SIZE)),
#endif
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
