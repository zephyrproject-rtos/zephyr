/*
 * Copyright (c) 2018 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <generated_dts_board.h>
#include <soc.h>
#include <arch/arc/v2/mpu/arc_mpu.h>
#include <linker/linker-defs.h>

#ifdef CONFIG_USERSPACE
static struct arc_mpu_region mpu_regions[] = {
	/* Region ICCM */
	MPU_REGION_ENTRY("ICCM",
			 DT_ICCM_BASE_ADDRESS,
			 DT_ICCM_SIZE * 1024,
			 REGION_FLASH_ATTR),
	/* Region DCCM */
	MPU_REGION_ENTRY("DCCM",
			 DT_DCCM_BASE_ADDRESS,
			 DT_DCCM_SIZE * 1024,
			 AUX_MPU_RDP_KW | AUX_MPU_RDP_KR),
	/* Region DDR RAM */
	MPU_REGION_ENTRY("SRAM",
			CONFIG_SRAM_BASE_ADDRESS,
			CONFIG_SRAM_SIZE * 1024,
			AUX_MPU_RDP_KW | AUX_MPU_RDP_KR |
			AUX_MPU_RDP_KE | AUX_MPU_RDP_UE),
	MPU_REGION_ENTRY("FLASH_0",
			CONFIG_FLASH_BASE_ADDRESS,
			CONFIG_FLASH_SIZE * 1024,
			AUX_MPU_RDP_KR |
			AUX_MPU_RDP_KE | AUX_MPU_RDP_UE),
	/* Region Peripheral */
	MPU_REGION_ENTRY("PERIPHERAL",
			 0xF0000000,
			 64 * 1024,
			 AUX_MPU_RDP_KW | AUX_MPU_RDP_KR),
};
#else /* CONFIG_USERSPACE */
static struct arc_mpu_region mpu_regions[] = {
	/* Region ICCM */
	MPU_REGION_ENTRY("ICCM",
			 DT_ICCM_BASE_ADDRESS,
			 DT_ICCM_SIZE * 1024,
			 REGION_FLASH_ATTR),
	/* Region DCCM */
	MPU_REGION_ENTRY("DCCM",
			 DT_DCCM_BASE_ADDRESS,
			 DT_DCCM_SIZE * 1024,
			 REGION_RAM_ATTR),
	MPU_REGION_ENTRY("FLASH_0",
			CONFIG_FLASH_BASE_ADDRESS,
			CONFIG_FLASH_SIZE * 1024,
			REGION_FLASH_ATTR),
	/* Region DDR RAM */
	MPU_REGION_ENTRY("SRAM",
			CONFIG_SRAM_BASE_ADDRESS,
			CONFIG_SRAM_SIZE * 1024,
			REGION_ALL_ATTR),
	/* Region Peripheral */
	MPU_REGION_ENTRY("PERIPHERAL",
			 0xF0000000,
			 64 * 1024,
			 REGION_IO_ATTR),
};
#endif

struct arc_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
