/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include "test_fs.h"

static struct test_fs_data test_data;

static struct fs_mount_t mp = {
		.type = TEST_FS_1,
		.mnt_point = TEST_FS_MNTP,
		.fs_data = &test_data,
};

void test_mount_flags(void)
{
	int ret = 0;
	struct fs_file_t fs = { 0 };

	/* Format volume and add some files/dirs to check read-only flag */
	mp.flags = 0;
	ret = fs_mount(&mp);
	TC_PRINT("Mount to prepare tests\n");
	zassert_equal(ret, 0, "Expected success", ret);
	TC_PRINT("Create some file\n");
	ret = fs_open(&fs, TEST_FS_MNTP"/some", FS_O_CREATE);
	zassert_equal(ret, 0, "Expected success", ret);
	fs_close(&fs);
	TC_PRINT("Create other directory\n");
	zassert_equal(ret, 0, "Expected success", ret);
	ret = fs_mkdir(TEST_FS_MNTP"/other");
	fs_unmount(&mp);

	/* Check fs operation on volume mounted with FS_MOUNT_FLAG_READ_ONLY */
	mp.flags = FS_MOUNT_FLAG_READ_ONLY;
	TC_PRINT("Mount as read-only\n");
	ret = fs_mount(&mp);
	zassert_equal(ret, 0, "Expected success", ret);

	/* Attempt creating new file */
	ret = fs_open(&fs, TEST_FS_MNTP"/nosome", FS_O_CREATE);
	zassert_equal(ret, -EROFS, "Expected EROFS", ret);
	ret = fs_mkdir(TEST_FS_MNTP"/another");
	zassert_equal(ret, -EROFS, "Expected EROFS", ret);
	ret = fs_rename(TEST_FS_MNTP"/some", TEST_FS_MNTP"/nosome");
	zassert_equal(ret, -EROFS, "Expected EROFS", ret);
	ret = fs_unlink(TEST_FS_MNTP"/some");
	zassert_equal(ret, -EROFS, "Expected EROFS", ret);
	ret = fs_open(&fs, TEST_FS_MNTP"/other", FS_O_CREATE);
	zassert_equal(ret, -EROFS, "Expected EROFS", ret);
	ret = fs_open(&fs, TEST_FS_MNTP"/some", FS_O_RDWR);
	zassert_equal(ret, -EROFS, "Expected EROFS", ret);
	ret = fs_open(&fs, TEST_FS_MNTP"/some", FS_O_READ);
	zassert_equal(ret, 0, "Expected success", ret);
	fs_close(&fs);
	fs_unmount(&mp);
}
