/*
 * Copyright (c) 2024 Siemens AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_ram_disk

#define RAMDISK_DISKIMG_EMBED_ONE(n)                                                               \
	__disk_buf_##n = .;                                                                        \
	*(STRINGIFY(.ramdisk_##n))                                                                 \
	__disk_buf_size_##n = ABSOLUTE(. - __disk_buf_##n);                                        \
	__disk_buf_sector_count_##n = ABSOLUTE(__disk_buf_size_##n / DT_INST_PROP(n, sector_size));

#define RAMDISK_DISKIMG_EMBED(n)                                                   	           \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, zephyr_disk_image_path),                              \
			(RAMDISK_DISKIMG_EMBED_ONE(n)), ())

DT_INST_FOREACH_STATUS_OKAY(RAMDISK_DISKIMG_EMBED)
