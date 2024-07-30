/*
 * Copyright (c) 2023 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/ext2.h>
#include "utils.h"

ZTEST(ext2tests, test_mount_only)
{
	struct fs_mount_t *mp = &testfs_mnt;
	int ret = 0;

	/* Test FS_MOUNT_FLAG_NO_FORMAT flag */
	mp->flags |= FS_MOUNT_FLAG_NO_FORMAT;
	ret = fs_mount(mp);
	TC_PRINT("Mount unformatted with FS_MOUNT_FLAG_NO_FORMAT set\n");
	zassert_false(ret == 0, "Expected failure (ret=%d)", ret);

	/* Test FS_MOUNT_FLAG_READ_ONLY on non-formatted volume*/
	mp->flags = FS_MOUNT_FLAG_READ_ONLY;
	ret = fs_mount(mp);
	TC_PRINT("Mount unformatted with FS_MOUNT_FLAG_READ_ONLY set\n");
	zassert_false(ret == 0, "Expected failure (ret=%d)", ret);

	/* Format volume and add some files/dirs to check read-only flag */
	mp->flags = 0;
	ret = fs_mount(mp);
	TC_PRINT("Mount again to format volume\n");
	zassert_equal(ret, 0, "Expected success (ret=%d)", ret);
	TC_PRINT("Create some file\n");

	ret = fs_unmount(mp);
	zassert_equal(ret, 0, "Expected success (ret=%d)", ret);

	/* Check fs operation on volume mounted with FS_MOUNT_FLAG_READ_ONLY */
	mp->flags = FS_MOUNT_FLAG_READ_ONLY;
	TC_PRINT("Mount as read-only\n");
	ret = fs_mount(mp);
	zassert_equal(ret, 0, "Expected success (ret=%d)", ret);

	ret = fs_unmount(mp);
	zassert_equal(ret, 0, "Expected success (ret=%d)", ret);
}

ZTEST(ext2tests, test_statvfs)
{
	int ret = 0;
	struct fs_statvfs sbuf;
	struct fs_mount_t *mp = &testfs_mnt;
	size_t partition_size = MIN(0x800000, get_partition_size((uintptr_t)mp->storage_dev));

	mp->flags = 0;
	ret = fs_mount(mp);
	zassert_equal(ret, 0, "Expected success (ret=%d)", ret);

	ret = fs_statvfs(mp->mnt_point, &sbuf);
	zassert_equal(ret, 0, "Expected success (ret=%d)", ret);

	TC_PRINT("Mounted file system: bsize:%lu frsize:%lu blocks:%lu, bfree:%lu\n",
			sbuf.f_bsize, sbuf.f_frsize, sbuf.f_blocks, sbuf.f_bfree);

	zassert_equal(sbuf.f_bsize, 1024,
			"Wrong block size %lu (expected %lu)", sbuf.f_bsize, 1024);
	zassert_equal(sbuf.f_frsize, 1024,
			"Wrong frag size %lu (expected %lu)", sbuf.f_frsize, 1024);
	zassert_equal(sbuf.f_blocks, partition_size / 1024,
			"Wrong block count %lu (expected %lu)",
			sbuf.f_blocks, partition_size / 1024);

	ret = fs_unmount(mp);
	zassert_equal(ret, 0, "Expected success (ret=%d)", ret);
}

/* Tests from common directory */

void test_fs_mkfs_simple(void);

/* Global variables expected by tests */
struct fs_mount_t *fs_mkfs_mp = &testfs_mnt;
const int fs_mkfs_type = FS_EXT2;
uintptr_t fs_mkfs_dev_id;
int fs_mkfs_flags;

ZTEST(ext2tests, test_mkfs_simple)
{
	struct fs_mount_t *mp = &testfs_mnt;

	fs_mkfs_dev_id = (uintptr_t) mp->storage_dev;
	fs_mkfs_flags = 0;
	test_fs_mkfs_simple();
}

void mkfs_custom_config(struct ext2_cfg *cfg)
{
	int ret = 0;
	struct fs_statvfs sbuf;
	struct fs_mount_t *mp = &testfs_mnt;
	size_t partition_size = MIN(cfg->fs_size, get_partition_size((uintptr_t)mp->storage_dev));

	ret = fs_mkfs(FS_EXT2, (uintptr_t)mp->storage_dev, cfg, 0);
	zassert_equal(ret, 0, "Failed to mkfs with 2K blocks");

	mp->flags = FS_MOUNT_FLAG_NO_FORMAT;
	ret = fs_mount(mp);
	zassert_equal(ret, 0, "Mount failed (ret=%d)", ret);

	ret = fs_statvfs(mp->mnt_point, &sbuf);
	zassert_equal(ret, 0, "Statvfs failed (ret=%d)", ret);

	TC_PRINT("Mounted file system: bsize:%lu frsize:%lu blocks:%lu, bfree:%lu\n",
			sbuf.f_bsize, sbuf.f_frsize, sbuf.f_blocks, sbuf.f_bfree);

	zassert_equal(sbuf.f_bsize, cfg->block_size,
			"Wrong block size %lu (expected %lu)", sbuf.f_bsize, cfg->block_size);
	zassert_equal(sbuf.f_frsize, cfg->block_size,
			"Wrong frag size %lu (expected %lu)", sbuf.f_frsize, cfg->block_size);
	zassert_equal(sbuf.f_blocks, partition_size / cfg->block_size,
			"Wrong block count %lu (expected %lu)",
			sbuf.f_blocks, partition_size / cfg->block_size);

	ret = fs_unmount(mp);
	zassert_equal(ret, 0, "Unmount failed (ret=%d)", ret);
}

#if defined(CONFIG_APP_TEST_BIG)
ZTEST(ext2tests, test_mkfs_custom_2K)
{
	struct ext2_cfg config = {
		.block_size = 2048,
		.fs_size = 0x2000000,
		.bytes_per_inode = 0,
		.volume_name[0] = 0,
		.set_uuid = false,
	};

	mkfs_custom_config(&config);
}

ZTEST(ext2tests, test_mkfs_custom_4K)
{
	struct ext2_cfg config = {
		.block_size = 4096,
		.fs_size = 0x8000000,
		.bytes_per_inode = 0,
		.volume_name[0] = 0,
		.set_uuid = false,
	};

	mkfs_custom_config(&config);
}
#endif
