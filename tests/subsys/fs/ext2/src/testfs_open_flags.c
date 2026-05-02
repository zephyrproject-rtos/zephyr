/*
 * Copyright (c) 2023 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/ext2.h>

#include "utils.h"

void test_fs_open_flags(void);

/* Expected by test_fs_open_flags() */
const char *test_fs_open_flags_file_path = "/sml/open_flags_file";

ZTEST(ext2tests, test_open_flags)
{
	struct fs_mount_t *mp = &testfs_mnt;

	zassert_equal(fs_mount(mp), 0, "Failed to mount partition");

	test_fs_open_flags();

	zassert_equal(fs_unmount(mp), 0, "Failed to unmount partition");
}

ZTEST(ext2tests, test_open_flags_2K)
{
	int ret = 0;
	struct fs_mount_t *mp = &testfs_mnt;
	struct ext2_cfg ext2_config = {
		.block_size = 2048,
		.fs_size = 0x2000000,
		.bytes_per_inode = 0,
		.volume_name[0] = 0,
		.set_uuid = false,
	};

	ret = fs_mkfs(FS_EXT2, (uintptr_t)mp->storage_dev, &ext2_config, 0);
	zassert_equal(ret, 0, "Failed to mkfs with 2K blocks");

	mp->flags = FS_MOUNT_FLAG_NO_FORMAT;
	zassert_equal(fs_mount(mp), 0, "Failed to mount partition");

	test_fs_open_flags();

	zassert_equal(fs_unmount(mp), 0, "Failed to unmount partition");
}

#if defined(CONFIG_APP_TEST_BIG)
ZTEST(ext2tests, test_open_flags_4K)
{
	int ret = 0;
	struct fs_mount_t *mp = &testfs_mnt;
	struct ext2_cfg ext2_config = {
		.block_size = 4096,
		.fs_size = 0x8000000,
		.bytes_per_inode = 0,
		.volume_name[0] = 0,
		.set_uuid = false,
	};

	ret = fs_mkfs(FS_EXT2, (uintptr_t)mp->storage_dev, &ext2_config, 0);
	zassert_equal(ret, 0, "Failed to mkfs with 2K blocks");

	mp->flags = FS_MOUNT_FLAG_NO_FORMAT;
	zassert_equal(fs_mount(mp), 0, "Failed to mount partition");

	test_fs_open_flags();

	zassert_equal(fs_unmount(mp), 0, "Failed to unmount partition");
}
#endif
