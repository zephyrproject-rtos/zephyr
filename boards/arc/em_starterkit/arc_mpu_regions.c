/*
 * Copyright (c) 2017 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <arch/arc/v2/mpu/arc_mpu.h>
#include <linker/linker-defs.h>

#ifdef CONFIG_USERSPACE
static struct arc_mpu_region mpu_regions[] = {
#if CONFIG_ARC_MPU_VER == 3 && defined(CONFIG_APPLICATION_MEMORY)
	/* Region ICCM */
	MPU_REGION_ENTRY("IMAGE ROM",
			 _image_rom_start,
			 _image_rom_end,
			 REGION_FLASH_ATTR),
	MPU_REGION_ENTRY("APP MEMORY",
			 __app_ram_start,
			 __app_ram_size,
			 REGION_RAM_ATTR),
	MPU_REGION_ENTRY("KERNEL MEMORY",
			 __kernel_ram_start,
			 __kernel_ram_size,
			 AUX_MPU_RDP_KW | AUX_MPU_RDP_KR),

#else
#if CONFIG_ICCM_SIZE > 0
	/* Region ICCM */
	MPU_REGION_ENTRY("ICCM",
			 CONFIG_ICCM_BASE_ADDRESS,
			 CONFIG_ICCM_SIZE * 1024,
			 REGION_FLASH_ATTR),
#endif
#if CONFIG_DCCM_SIZE > 0
	/* Region DCCM */
	MPU_REGION_ENTRY("DCCM",
			 CONFIG_DCCM_BASE_ADDRESS,
			 CONFIG_DCCM_SIZE * 1024,
			 AUX_MPU_RDP_KW | AUX_MPU_RDP_KR),
#endif
#if CONFIG_SRAM_SIZE > 0
	/* Region DDR RAM */
	MPU_REGION_ENTRY("DDR RAM",
			CONFIG_SRAM_BASE_ADDRESS,
			CONFIG_SRAM_SIZE * 1024,
			AUX_MPU_RDP_KW | AUX_MPU_RDP_KR |
			AUX_MPU_RDP_KE | AUX_MPU_RDP_UE),
#endif
#endif /* ARC_MPU_VER == 3 */
	/* Region Peripheral */
	MPU_REGION_ENTRY("PERIPHERAL",
			 0xF0000000,
			 64 * 1024,
			 AUX_MPU_RDP_KW | AUX_MPU_RDP_KR),
};
#else
static struct arc_mpu_region mpu_regions[] = {
#if CONFIG_ICCM_SIZE > 0
	/* Region ICCM */
	MPU_REGION_ENTRY("ICCM",
			 CONFIG_ICCM_BASE_ADDRESS,
			 CONFIG_ICCM_SIZE * 1024,
			 REGION_FLASH_ATTR),
#endif
#if CONFIG_DCCM_SIZE > 0
	/* Region DCCM */
	MPU_REGION_ENTRY("DCCM",
			 CONFIG_DCCM_BASE_ADDRESS,
			 CONFIG_DCCM_SIZE * 1024,
			 REGION_RAM_ATTR),
#endif
#if CONFIG_SRAM_SIZE > 0
	/* Region DDR RAM */
	MPU_REGION_ENTRY("DDR RAM",
			CONFIG_SRAM_BASE_ADDRESS,
			CONFIG_SRAM_SIZE * 1024,
			REGION_ALL_ATTR),
#endif
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
