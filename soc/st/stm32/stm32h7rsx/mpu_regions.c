/*
 * Copyright (c) 2017 Linaro Limited.
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/slist.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>

#include <zephyr/arch/arm/mpu/arm_mpu_mem_cfg.h>

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(mac))
#define sram_eth_node DT_PHANDLE(DT_NODELABEL(mac), memory_regions)

BUILD_ASSERT(!DT_SAME_NODE(sram_eth_node, DT_CHOSEN(zephyr_sram)),
	     "Ethernet buffer memory-region cannot be located in Zephyr system RAM.");
#endif /* mac node enabled */

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

	/* Region 4 - OTP area */
	MPU_REGION_ENTRY("OTP", 0x08FFF000, REGION_IO_ATTR(REGION_1K)),

	/* Region 5 - Read-only area provisioned by ST */
	MPU_REGION_ENTRY("ID", 0x08FFF800, REGION_IO_ATTR(REGION_512B)),

#if defined(sram_eth_node) && DT_NODE_HAS_STATUS_OKAY(sram_eth_node)
	/* Region 6 - Ethernet DMA buffer RAM */
	MPU_REGION_ENTRY("SRAM_ETH_BUF", DT_REG_ADDR(sram_eth_node),
			 REGION_RAM_NOCACHE_ATTR(REGION_16K)),
	/* Region 7 - Ethernet DMA descriptor RAM (overlays the first 256B of SRAM_ETH_BUF) */
	MPU_REGION_ENTRY("SRAM_ETH_DESC", DT_REG_ADDR(sram_eth_node), REGION_PPB_ATTR(REGION_256B)),
#endif
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
