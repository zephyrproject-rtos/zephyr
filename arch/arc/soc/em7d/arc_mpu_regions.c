/*
 * Copyright (c) 2017 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <soc.h>
 #include <arch/arc/v2/mpu/arc_mpu.h>

static struct arc_mpu_region mpu_regions[] = {
#if CONFIG_ICCM_SIZE > 0
	/* Region ICCM */
	MPU_REGION_ENTRY("ICCM",
			 CONFIG_ICCM_BASE_ADDRESS,
			 REGION_FLASH_ATTR(REGION_256K)),
#endif
#if CONFIG_DCCM_SIZE > 0
	/* Region DCCM */
	MPU_REGION_ENTRY("DCCM",
			 CONFIG_DCCM_BASE_ADDRESS,
			 REGION_RAM_ATTR(REGION_128K)),
#endif
#if CONFIG_SRAM_SIZE > 0
	/* Region DDR RAM */
	MPU_REGION_ENTRY("DDR RAM",
			CONFIG_SRAM_BASE_ADDRESS,
			REGION_ALL_ATTR(REGION_128M)),
#endif
	/* Region Peripheral */
	MPU_REGION_ENTRY("PERIPHERAL",
			 0xF0000000,
			 REGION_IO_ATTR(REGION_64K)),
};

struct arc_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
