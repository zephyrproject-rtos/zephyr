/*
 * Copyright (c) 2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/fs/littlefs.h>
#include "testfs_tests.h"
#include "testfs_lfs.h"

void test_fs_mkfs_ops(void);
void test_fs_mkfs_simple(void);
/* Using smallest partition for this tests as they do not write
 * a lot of data, basically they just check flags.
 */
struct fs_mount_t *fs_mkfs_mp = &testfs_small_mnt;
const int fs_mkfs_type = FS_LITTLEFS;
uintptr_t fs_mkfs_dev_id;
int fs_mkfs_flags;
const char *some_file_path = "/sml/some";
const char *other_dir_path = "/sml/other";

static void cleanup(struct fs_mount_t *mp)
{
	TC_PRINT("Clean %s\n", mp->mnt_point);

	zassert_equal(testfs_lfs_wipe_partition(mp), TC_PASS,
		      "Failed to clean partition");
}

ZTEST(littlefs, test_fs_mkfs_simple_lfs)
{
	cleanup(fs_mkfs_mp);

	fs_mkfs_dev_id = (uintptr_t) testfs_small_mnt.storage_dev;
	test_fs_mkfs_simple();
}

ZTEST(littlefs, test_fs_mkfs_ops_lfs)
{
	cleanup(fs_mkfs_mp);

	fs_mkfs_dev_id = (uintptr_t) testfs_small_mnt.storage_dev;
	test_fs_mkfs_ops();
}

/* Custom config with doubled the prog size */
FS_LITTLEFS_DECLARE_CUSTOM_CONFIG(custom_cfg,
		CONFIG_FS_LITTLEFS_READ_SIZE,
		CONFIG_FS_LITTLEFS_PROG_SIZE * 2,
		CONFIG_FS_LITTLEFS_CACHE_SIZE,
		CONFIG_FS_LITTLEFS_LOOKAHEAD_SIZE);

ZTEST(littlefs, test_fs_mkfs_custom)
{
	int ret = 0;
	struct fs_statvfs sbuf;
	struct fs_mount_t mnt = testfs_small_mnt;

	cleanup(fs_mkfs_mp);

	ret = fs_mkfs(FS_LITTLEFS, (uintptr_t)testfs_small_mnt.storage_dev, &custom_cfg, 0);
	zassert_equal(ret, 0, "Expected success (ret=%d)", ret);

	mnt.flags = FS_MOUNT_FLAG_NO_FORMAT;
	mnt.fs_data = &custom_cfg;
	ret = fs_mount(&mnt);
	zassert_equal(ret, 0, "Expected success (ret=%d)", ret);

	ret = fs_statvfs(mnt.mnt_point, &sbuf);
	zassert_equal(ret, 0, "Expected success (ret=%d)", ret);

	TC_PRINT("f_bsize= %lu", sbuf.f_bsize);
	/* Prog size is returned in f_bsize field. */
	zassert_equal(sbuf.f_bsize, 2 * CONFIG_FS_LITTLEFS_PROG_SIZE);

	ret = fs_unmount(&mnt);
	zassert_equal(ret, 0, "Expected success (ret=%d)", ret);
}
