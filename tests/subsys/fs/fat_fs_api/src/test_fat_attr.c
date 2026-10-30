/*
 * Copyright (c) 2026 Rishav Chakraborty
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_fat.h"
#include <string.h>

static void write_test_file(fs_mode_t flags)
{
	ssize_t brw;
	int res;

	res = fs_open(&filep, TEST_FILE, flags);
	zassert_equal(res, 0, "fs_open failed [%d]", res);

	brw = fs_write(&filep, test_str, strlen(test_str));
	zassert_equal(brw, strlen(test_str), "fs_write failed [%zd]", brw);

	res = fs_close(&filep);
	zassert_equal(res, 0, "fs_close failed [%d]", res);
}

static void test_attr_mtime(void)
{
	struct fs_attr before;
	struct fs_attr after;
	int res;

	if (!IS_ENABLED(CONFIG_FS_FATFS_HAS_RTC)) {
		TC_PRINT("FatFs timestamping disabled, skipping mtime test\n");
		return;
	}

	write_test_file(FS_O_CREATE | FS_O_RDWR);

	res = fs_getattr(TEST_FILE, &before);
	zassert_equal(res, 0, "fs_getattr failed [%d]", res);
	zassert_true((before.mask & FS_ATTR_MTIME) != 0U, "mtime not supported");

	/* FAT timestamps have 2 second resolution */
	k_sleep(K_SECONDS(2));

	write_test_file(FS_O_RDWR);

	res = fs_getattr(TEST_FILE, &after);
	zassert_equal(res, 0, "fs_getattr failed [%d]", res);
	zassert_true(after.mtime > before.mtime, "mtime did not advance (%lld, %lld)",
		     (long long)before.mtime, (long long)after.mtime);

	res = fs_unlink(TEST_FILE);
	zassert_equal(res, 0, "fs_unlink failed [%d]", res);
}

static void test_attr_mount_point(void)
{
	struct fs_attr attr;
	int res;

	/* The FAT root directory has no timestamp */
	res = fs_getattr(FATFS_MNTP, &attr);
	zassert_equal(res, 0, "fs_getattr on mount point failed [%d]", res);
	zassert_equal(attr.mtime, 0, "Unexpected mtime %lld", (long long)attr.mtime);
}

void test_fat_attr(void)
{
	test_attr_mtime();
	test_attr_mount_point();
}
