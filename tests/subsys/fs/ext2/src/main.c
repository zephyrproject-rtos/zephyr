/*
 * Copyright (c) 2023 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>

#include "test_disk.h"
#include "utils.h"

#ifdef CONFIG_DISK_DRIVER_RAM
#define STORAGE_DEVICE EXT2_TEST_DISK_NAME
#elif CONFIG_DISK_DRIVER_FLASH
#define STORAGE_DEVICE "NAND"
#elif CONFIG_DISK_DRIVER_SDMMC
#define STORAGE_DEVICE "SD"
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

static void *setup_suite(void)
{
#ifdef CONFIG_DISK_DRIVER_RAM
	zassert_equal(ext2_test_disk_register(), 0, "Failed to register the EXT2 test disk");
#endif

	return NULL;
}

static void before_test(void *f)
{
	ARG_UNUSED(f);

#ifdef CONFIG_DISK_DRIVER_RAM
	zassert_equal(ext2_test_disk_reset(), 0, "Failed to reset the EXT2 test disk");
#endif

	zassert_equal(wipe_partition((uintptr_t)testfs_mnt.storage_dev), TC_PASS,
		      "Failed to clean partition");

#ifdef CONFIG_DISK_DRIVER_RAM
	zassert_equal(ext2_test_disk_reset(), 0,
		      "Failed to release the EXT2 test disk after wiping");
#endif

	testfs_mnt.flags = 0;
}

static void after_test(void *f)
{
	ARG_UNUSED(f);

	/* Unmount file system if the test left it mounted. */
	fs_unmount(&testfs_mnt);

#ifdef CONFIG_DISK_DRIVER_RAM
	zassert_equal(ext2_test_disk_reset(), 0, "Failed to reset the EXT2 test disk");
#endif
}

static void teardown_suite(void *f)
{
	ARG_UNUSED(f);

#ifdef CONFIG_DISK_DRIVER_RAM
	zassert_equal(ext2_test_disk_reset(), 0, "Failed to reset the EXT2 test disk");
	zassert_equal(ext2_test_disk_unregister(), 0, "Failed to unregister the EXT2 test disk");
#endif
}

ZTEST_SUITE(ext2tests, NULL, setup_suite, before_test, after_test, teardown_suite);
