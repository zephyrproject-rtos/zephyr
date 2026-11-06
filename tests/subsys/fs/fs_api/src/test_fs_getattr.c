/*
 * Copyright (c) 2026 Rishav Chakraborty
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_fs.h"

#if defined(CONFIG_FILE_SYSTEM_GETATTR)

static struct test_fs_data test_data;
static struct fs_mount_t test_fs_mnt = {
	.type = TEST_FS_1,
	.mnt_point = TEST_FS_MNTP,
	.fs_data = &test_data,
};

static void *fs_getattr_setup(void)
{
	fs_register(TEST_FS_1, &temp_fs);
	fs_mount(&test_fs_mnt);

	return NULL;
}

static void fs_getattr_teardown(void *fixture)
{
	fs_unmount(&test_fs_mnt);
	fs_unregister(TEST_FS_1, &temp_fs);
}

ZTEST(fs_api_getattr, test_fs_getattr)
{
	struct fs_attr attr;
	int ret;

	ret = fs_getattr(TEST_FILE, &attr);
	zassert_equal(ret, 0, "fs_getattr failed (%d)", ret);
	zassert_equal(attr.mask, FS_ATTR_MTIME, "Wrong attribute mask 0x%x", attr.mask);
	zassert_equal(attr.mtime, TEST_MTIME, "Wrong mtime %lld", (long long)attr.mtime);

	ret = fs_getattr(NULL, &attr);
	zassert_equal(ret, -EINVAL, "NULL path (%d)", ret);

	ret = fs_getattr("relative/path", &attr);
	zassert_equal(ret, -EINVAL, "Relative path (%d)", ret);

	ret = fs_getattr(TEST_FILE, NULL);
	zassert_equal(ret, -EINVAL, "NULL attr (%d)", ret);
}

ZTEST_SUITE(fs_api_getattr, NULL, fs_getattr_setup, NULL, NULL, fs_getattr_teardown);
#endif /* CONFIG_FILE_SYSTEM_GETATTR */
