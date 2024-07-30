/*
 * Copyright (c) 2023 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>

#include "utils.h"

#ifdef CONFIG_DISK_DRIVER_RAM
	#define STORAGE_DEVICE "RAM"
#elif CONFIG_DISK_DRIVER_FLASH
	#define STORAGE_DEVICE "NAND"
#elif CONFIG_DISK_DRIVER_SDMMC
	#define STORAGE_DEVICE "SDMMC"
#endif

/* All tests must use this structure to mount file system. After each test this structure is cleaned
 * to allow for running next tests unaffected by previous one.
 */
struct fs_mount_t testfs_mnt = {
	.type = FS_EXT2,
	.mnt_point = "/sml",
	.storage_dev = STORAGE_DEVICE,
	.flags = 0,
};

static void before_test(void *f)
{
	ARG_UNUSED(f);

	zassert_equal(wipe_partition((uintptr_t)testfs_mnt.storage_dev), TC_PASS,
			"Failed to clean partition");
	testfs_mnt.flags = 0;
}

static void after_test(void *f)
{
	ARG_UNUSED(f);

	/* Unmount file system */
	fs_unmount(&testfs_mnt);
}

ZTEST_SUITE(ext2tests, NULL, NULL, before_test, after_test, NULL);
