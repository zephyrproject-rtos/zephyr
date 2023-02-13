/*
 * Copyright (c) 2022 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>

#if defined(CONFIG_FILE_SYSTEM_LITTLEFS) && defined(CONFIG_FAT_FILESYSTEM_ELM)
#error "Only one file system may be used at once in that sample."
#endif


#if defined(CONFIG_FILE_SYSTEM_LITTLEFS)

#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/littlefs.h>

#define MKFS_FS_TYPE FS_LITTLEFS
#define MKFS_DEV_ID FIXED_PARTITION_ID(storage_partition)
#define MKFS_FLAGS 0

#elif defined(CONFIG_FAT_FILESYSTEM_ELM)

#include <zephyr/storage/disk_access.h>
#include <ff.h>

#define MKFS_FS_TYPE FS_FATFS
#define MKFS_DEV_ID "RAM:"
#define MKFS_FLAGS 0

#else
#error "No filesystem specified."
#endif

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

void main(void)
{
	int rc;

	rc = fs_mkfs(MKFS_FS_TYPE, (uintptr_t)MKFS_DEV_ID, NULL, MKFS_FLAGS);

	if (rc < 0) {
		LOG_ERR("Format failed");
		return;
	}

	LOG_INF("Format successful");
}
