/*
 * Copyright (c) 2017 Linaro Limited.
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/slist.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>

#include <zephyr/arch/arm/mpu/arm_mpu_mem_cfg.h>

static const struct arm_mpu_region mpu_regions[] = {
	/* Use first region to prevent speculative access in entire memory space */
	/* Region 0 */
	MPU_REGION_ENTRY("UNMAPPED", 0, {REGION_4G | MPU_RASR_XN_Msk | P_NA_U_NA_Msk}),

	/* Region 1 */
	MPU_REGION_ENTRY("PERIPH", 0x40000000, REGION_IO_ATTR(REGION_512M)),

#ifdef CONFIG_XIP
	/* Region 2 */
	MPU_REGION_ENTRY("FLASH_0", CONFIG_FLASH_BASE_ADDRESS,
			 REGION_FLASH_ATTR(REGION_FLASH_SIZE)),
#endif

	/* Region 3 */
	MPU_REGION_ENTRY("SRAM_0", CONFIG_SRAM_BASE_ADDRESS, REGION_RAM_ATTR(REGION_SRAM_SIZE)),

	/* Region 4 - Ready only flash with unique device id */
	MPU_REGION_ENTRY("ID", 0x08FFF800, REGION_FLASH_ATTR(REGION_2K)),

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(mac))
#define sram_eth_node DT_NODELABEL(sram2)
#if DT_NODE_HAS_STATUS_OKAY(sram_eth_node)
	/* Region 5 - Ethernet DMA buffer RAM */
	MPU_REGION_ENTRY("SRAM_ETH_BUF", DT_REG_ADDR(sram_eth_node),
			 REGION_RAM_NOCACHE_ATTR(REGION_16K)),
	/* Region 6 - Ethernet DMA descriptor RAM (overlays the first 256B of SRAM_ETH_BUF)*/
	MPU_REGION_ENTRY("SRAM_ETH_DESC", DT_REG_ADDR(sram_eth_node), REGION_PPB_ATTR(REGION_256B)),
#endif
#endif
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
