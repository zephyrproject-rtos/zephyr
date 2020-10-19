/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <fs/littlefs.h>
#include "testfs_tests.h"
#include "testfs_lfs.h"

static void cleanup(struct fs_mount_t *mp)
{
	TC_PRINT("Clean %s\n", mp->mnt_point);

	zassert_equal(testfs_lfs_wipe_partition(mp), TC_PASS,
		      "Failed to clean partition");
}

void test_fs_mount_flags(void)
{
	/* Using smallest partition for this tests as they do not write
	 * a lot of data, basically they just check flags.
	 */
	struct fs_mount_t *mp = &testfs_small_mnt;
	int ret = 0;
	struct fs_file_t fs;

	cleanup(mp);

	/* Test FS_MOUNT_FLAG_NO_FORMAT flag */
	mp->flags |= FS_MOUNT_FLAG_NO_FORMAT;
	ret = fs_mount(mp);
	TC_PRINT("Mount unformatted with FS_MOUNT_FLAG_NO_FORMAT set\n");
	zassert_false(ret == 0, "Expected failure", ret);

	/* Test FS_MOUNT_FLAG_READ_ONLY on non-formatted volume*/
	mp->flags = FS_MOUNT_FLAG_READ_ONLY;
	ret = fs_mount(mp);
	TC_PRINT("Mount unformatted with FS_MOUNT_FLAG_READ_ONLY set\n");
	zassert_false(ret == 0, "Expected failure", ret);

	/* Format volume and add some files/dirs to check read-only flag */
	mp->flags = 0;
	ret = fs_mount(mp);
	TC_PRINT("Mount again to format volume\n");
	zassert_equal(ret, 0, "Expected success", ret);
	TC_PRINT("Create some file\n");
	ret = fs_open(&fs, "/sml/some", FS_O_CREATE);
	zassert_equal(ret, 0, "Expected success", ret);
	fs_close(&fs);
	TC_PRINT("Create other directory\n");
	zassert_equal(ret, 0, "Expected success", ret);
	ret = fs_mkdir("/sml/other");
	fs_unmount(mp);

	/* Check fs operation on volume mounted with FS_MOUNT_FLAG_READ_ONLY */
	mp->flags = FS_MOUNT_FLAG_READ_ONLY;
	TC_PRINT("Mount as read-only\n");
	ret = fs_mount(mp);
	zassert_equal(ret, 0, "Expected success", ret);

	/* Attempt creating new file */
	ret = fs_open(&fs, "/sml/nosome", FS_O_CREATE);
	zassert_equal(ret, -EROFS, "Expected EROFS", ret);
	ret = fs_mkdir("/sml/another");
	zassert_equal(ret, -EROFS, "Expected EROFS", ret);
	ret = fs_rename("/sml/some", "/sml/nosome");
	zassert_equal(ret, -EROFS, "Expected EROFS", ret);
	ret = fs_unlink("/sml/some");
	zassert_equal(ret, -EROFS, "Expected EROFS", ret);
	ret = fs_open(&fs, "/sml/other", FS_O_CREATE);
	zassert_equal(ret, -EROFS, "Expected EROFS", ret);
	ret = fs_open(&fs, "/sml/some", FS_O_RDWR);
	zassert_equal(ret, -EROFS, "Expected EROFS", ret);
	ret = fs_open(&fs, "/sml/some", FS_O_READ);
	zassert_equal(ret, 0, "Expected success", ret);
	fs_close(&fs);
	fs_unmount(mp);
}
