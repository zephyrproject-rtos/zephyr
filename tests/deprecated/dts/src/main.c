/*
 * Copyright (c) 2020 Linaro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <storage/flash_map.h>

void main(void)
{
	BUILD_ASSERT(DT_FLASH_AREA_MCUBOOT_ID == FLASH_AREA_ID(mcuboot),
		     "FLASH AREA ID mismatch for MCUBOOT partition");
	/* disabled status is ignored for partitions */
	BUILD_ASSERT(DT_FLASH_AREA_STORAGE_ID == FLASH_AREA_ID(storage),
		     "FLASH AREA ID mismatch for STORAGE partition");
	BUILD_ASSERT(DT_FLASH_AREA_IMAGE_0_ID == FLASH_AREA_ID(image_0),
		     "FLASH AREA ID mismatch for IMAGE_0 partition");
	BUILD_ASSERT(DT_FLASH_AREA_IMAGE_1_ID == FLASH_AREA_ID(image_1),
		     "FLASH AREA ID mismatch for IMAGE_1 partition");
	BUILD_ASSERT(DT_FLASH_AREA_IMAGE_SCRATCH_ID ==
		     FLASH_AREA_ID(image_scratch),
		     "FLASH AREA ID mismatch for IMAGE_SCRATCH partition");
}
