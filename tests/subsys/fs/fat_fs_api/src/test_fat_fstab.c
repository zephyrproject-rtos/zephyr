/*
 * Copyright (c) 2026 Meta Platforms, Inc. and affiliates.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/storage/disk_access.h>

#include "test_fat.h"

#define TEST_FSTAB DT_NODELABEL(test_fstab)

FS_FSTAB_DECLARE_ENTRY(TEST_FSTAB);

ZTEST(fat_fs_fstab, test_fstab_no_format)
{
	struct fs_mount_t *mp = &FS_FSTAB_ENTRY(TEST_FSTAB);
	static uint8_t sector[512];

	zassert_equal(mp->flags & FS_MOUNT_FLAG_NO_FORMAT, FS_MOUNT_FLAG_NO_FORMAT);
	zassert_ok(wipe_partition());
	zassert_false(fs_mount(mp) == 0, "expected mount fail due to not existing file system");

	zassert_ok(disk_access_read(DISK_NAME, sector, 0, 1));
	zassert_false((sector[510] == 0x55U) && (sector[511] == 0xAAU), "disk was formatted");
}

ZTEST_SUITE(fat_fs_fstab, NULL, NULL, NULL, NULL, NULL);
