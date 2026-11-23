/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
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
	struct fs_file_t fs;

	fs_file_t_init(&fs);

	/* Format volume and add some files/dirs to check read-only flag */
	mp.flags = 0;
	TC_PRINT("Mount to prepare tests\n");
	ret = fs_mount(&mp);
	zassert_equal(ret, 0, "Expected success (%d)", ret);
	TC_PRINT("Create some file\n");
	ret = fs_open(&fs, TEST_FS_MNTP"/some", FS_O_CREATE);
	zassert_equal(ret, 0, "Expected success fs_open(FS_O_CREATE) (%d)",
		      ret);
	ret = fs_close(&fs);
	zassert_equal(ret, 0, "Expected fs_close success (%d)", ret);
	TC_PRINT("Create other directory\n");
	ret = fs_mkdir(TEST_FS_MNTP"/other");
	zassert_equal(ret, 0, "Expected fs_mkdir success (%d)", ret);
	ret = fs_unmount(&mp);
	zassert_equal(ret, 0, "Expected fs_umount success (%d)", ret);

	/* Check fs operation on volume mounted with FS_MOUNT_FLAG_READ_ONLY */
	mp.flags = FS_MOUNT_FLAG_READ_ONLY;
	TC_PRINT("Mount as read-only\n");
	ret = fs_mount(&mp);
	zassert_equal(ret, 0, "Expected fs_mount success (%d)", ret);

	/* Attempt creating new file */
	ret = fs_open(&fs, TEST_FS_MNTP"/nosome", FS_O_CREATE);
	zassert_equal(ret, -EROFS, "Expected EROFS got %d", ret);
	ret = fs_mkdir(TEST_FS_MNTP"/another");
	zassert_equal(ret, -EROFS, "Expected EROFS got %d", ret);
	ret = fs_rename(TEST_FS_MNTP"/some", TEST_FS_MNTP"/nosome");
	zassert_equal(ret, -EROFS, "Expected EROFS got %d", ret);
	ret = fs_unlink(TEST_FS_MNTP"/some");
	zassert_equal(ret, -EROFS, "Expected EROFS got %d", ret);
	ret = fs_open(&fs, TEST_FS_MNTP"/other", FS_O_CREATE);
	zassert_equal(ret, -EROFS, "Expected EROFS got %d", ret);
	ret = fs_open(&fs, TEST_FS_MNTP"/some", FS_O_RDWR);
	zassert_equal(ret, -EROFS, "Expected EROFS got %d", ret);
	ret = fs_open(&fs, TEST_FS_MNTP"/some", FS_O_READ);
	zassert_equal(ret, 0, "Expected fs_open(FS_O_READ) success (%d)", ret);
	ret = fs_close(&fs);
	zassert_equal(ret, 0, "Expected fs_close success (%d)", ret);
	ret = fs_unmount(&mp);
	zassert_equal(ret, 0, "Expected fs_unmount success (%d)", ret);
}
