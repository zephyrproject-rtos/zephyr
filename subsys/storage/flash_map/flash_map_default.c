/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <flash_map.h>

const struct flash_area default_flash_map[] = {
	/* FLASH_AREA_BOOTLOADER */
	{
		.fa_id = 0,
		.fa_device_id = SOC_FLASH_0_ID,
		.fa_off = FLASH_AREA_MCUBOOT_OFFSET,
		.fa_size = FLASH_AREA_MCUBOOT_SIZE,
	},

	/* FLASH_AREA_IMAGE_0 */
	{
		.fa_id = 1,
		.fa_device_id = SOC_FLASH_0_ID,
		.fa_off = FLASH_AREA_IMAGE_0_OFFSET,
		.fa_size = FLASH_AREA_IMAGE_0_SIZE,
	},

	/* FLASH_AREA_IMAGE_1 */
	{
		.fa_id = 2,
		.fa_device_id = SOC_FLASH_0_ID,
		.fa_off = FLASH_AREA_IMAGE_1_OFFSET,
		.fa_size = FLASH_AREA_IMAGE_1_SIZE,
	},

	/* FLASH_AREA_IMAGE_SCRATCH */
	{
		.fa_id = 3,
		.fa_device_id = SOC_FLASH_0_ID,
		.fa_off = FLASH_AREA_IMAGE_SCRATCH_OFFSET,
		.fa_size = FLASH_AREA_IMAGE_SCRATCH_SIZE,
	},

#ifdef CONFIG_FILE_SYSTEM_NFFS
	/* FLASH_AREA_NFFS_ */
	{
		.fa_id = 4,
		.fa_device_id = SOC_FLASH_0_ID,
		.fa_off = FLASH_AREA_NFFS_OFFSET,
		.fa_size = FLASH_AREA_NFFS_SIZE,
	},
#endif
};

const int flash_map_entries = ARRAY_SIZE(default_flash_map);
const struct flash_area *flash_map = default_flash_map;
