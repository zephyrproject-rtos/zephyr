/*
 * Copyright (c) 2026 Tawonga Kondolo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/ztest.h>

#include "test_disk.h"

#if defined(CONFIG_DISK_DRIVER_RAM) && defined(CONFIG_FS_LITTLEFS_BLK_DEV)

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(blk_fs);

static struct fs_mount_t littlefs_blk_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &blk_fs,
	.storage_dev = (void *)LITTLEFS_TEST_DISK_NAME,
	.mnt_point = "/blk",
	.flags = FS_MOUNT_FLAG_USE_DISK_ACCESS,
};

static void *setup_suite(void)
{
	zassert_equal(littlefs_test_disk_register(), 0,
		      "Failed to register the LittleFS test disk");

	return NULL;
}

static void before_test(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_equal(littlefs_test_disk_reset(), 0, "Failed to reset the LittleFS test disk");
}

static void after_test(void *fixture)
{
	ARG_UNUSED(fixture);

	(void)fs_unmount(&littlefs_blk_mnt);

	zassert_equal(littlefs_test_disk_reset(), 0, "Failed to reset the LittleFS test disk");
}

static void teardown_suite(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_equal(littlefs_test_disk_reset(), 0, "Failed to reset the LittleFS test disk");
	zassert_equal(littlefs_test_disk_unregister(), 0,
		      "Failed to unregister the LittleFS test disk");
}

ZTEST_SUITE(littlefs_blk, NULL, setup_suite, before_test, after_test, teardown_suite);

ZTEST(littlefs_blk, test_unmount_deinitializes_disk)
{
	int ret;

	for (int cycle = 0; cycle < 2; cycle++) {
		zassert_equal(littlefs_test_disk_refcount(), 0U,
			      "Disk reference remains before mount cycle %d", cycle);

		ret = fs_mount(&littlefs_blk_mnt);
		zassert_equal(ret, 0, "Mount cycle %d failed: %d", cycle, ret);
		zassert_equal(littlefs_test_disk_refcount(), 1U,
			      "Expected one disk reference after mount cycle %d", cycle);

		ret = fs_unmount(&littlefs_blk_mnt);
		zassert_equal(ret, 0, "Unmount cycle %d failed: %d", cycle, ret);
		zassert_equal(littlefs_test_disk_refcount(), 0U,
			      "Disk reference remains after unmount cycle %d", cycle);
	}
}

#endif /* CONFIG_DISK_DRIVER_RAM && CONFIG_FS_LITTLEFS_BLK_DEV */
