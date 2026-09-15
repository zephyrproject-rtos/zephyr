/*
 * Copyright (c) 2026 Dhruv Menon <dhruvmenon1104@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/ext2.h>

#include "utils.h"

/* 1024-byte blocks, 200-byte names -> 208-byte dirents, 4 entries per block.
 * Block 0 also holds "." and "..", so it still fits 4 files.
 * Direct blocks cover files 0..47; the first indirect block starts at 48.
 */
#define DIR_NAME_LEN    200
#define NFILES          56
#define INDIRECT_FIRST  48
#define INDIRECT_LAST   51
#define SURVIVOR        52

static int create_file(const char *path)
{
	struct fs_file_t file;
	int rc;

	fs_file_t_init(&file);
	rc = fs_open(&file, path, FS_O_CREATE);
	if (rc < 0) {
		return rc;
	}
	return fs_close(&file);
}

static void make_dir_path(char *buf, size_t buflen, unsigned int n)
{
	char name[DIR_NAME_LEN + 1];
	int len;

	len = snprintk(name, sizeof(name), "f%03u", n);
	memset(name + len, 'x', DIR_NAME_LEN - len);
	name[DIR_NAME_LEN] = '\0';
	snprintk(buf, buflen, "/sml/d/%s", name);
}

ZTEST(ext2tests, test_root_dot_unlink_rename)
{
	struct fs_mount_t *mp = &testfs_mnt;
	struct fs_dirent stat;

	zassert_equal(fs_mount(mp), 0, "Mount failed");

	zassert_equal(fs_rename("/sml", "/sml/moved"), -EINVAL,
			"renaming the filesystem root must fail");
	zassert_equal(fs_unlink("/sml"), -EINVAL, "unlinking the filesystem root must fail");
	zassert_equal(fs_unlink("/sml/."), -EINVAL, "unlink \".\" must fail");
	zassert_equal(fs_unlink("/sml/.."), -EINVAL, "unlink \"..\" must fail");

	zassert_equal(fs_mkdir("/sml/dir"), 0, "mkdir failed");
	zassert_equal(fs_unlink("/sml/dir/."), -EINVAL, "unlink dir/\".\" must fail");
	zassert_equal(fs_unlink("/sml/dir/.."), -EINVAL, "unlink dir/\"..\" must fail");
	zassert_equal(fs_rename("/sml/dir", "/sml/."), -EINVAL,
			"rename onto \".\" must fail");
	zassert_equal(fs_rename("/sml/dir/.", "/sml/x"), -EINVAL,
			"rename of \".\" must fail");

	zassert_equal(fs_stat("/sml/dir", &stat), 0, "directory should still exist");
	zassert_equal(fs_unmount(mp), 0, "Unmount failed");
}

ZTEST(ext2tests, test_indirect_dir_unlink)
{
	struct fs_mount_t *mp = &testfs_mnt;
	/* 128 KiB stays in one block group on the 128 MiB flash and 9 MiB
	 * ramdisk variants. fs_size=0 uses the whole device and mkfs then
	 * returns -ENOTSUP. Dense inodes also need a small volume so
	 * s_inodes_count fits in one bitmap block.
	 */
	struct ext2_cfg cfg = {
		.block_size = 1024,
		.fs_size = 0x20000,
		.bytes_per_inode = 128,
		.volume_name = {'e', 'x', 't', '2', '\0'},
		.set_uuid = false,
	};
	char path[256];
	struct fs_dirent stat;
	int rc;

	zassert_equal(fs_mkfs(FS_EXT2, (uintptr_t)mp->storage_dev, &cfg, 0), 0,
			"mkfs failed");

	mp->flags = FS_MOUNT_FLAG_NO_FORMAT;
	zassert_equal(fs_mount(mp), 0, "Mount failed");
	zassert_equal(fs_mkdir("/sml/d"), 0, "mkdir /sml/d failed");

	for (unsigned int i = 0; i < NFILES; i++) {
		make_dir_path(path, sizeof(path), i);
		rc = create_file(path);
		zassert_equal(rc, 0, "create %u failed: %d", i, rc);
	}

	make_dir_path(path, sizeof(path), SURVIVOR);
	zassert_equal(fs_stat(path, &stat), 0, "survivor must exist before unlink");

	for (unsigned int i = INDIRECT_FIRST + 1; i <= INDIRECT_LAST; i++) {
		make_dir_path(path, sizeof(path), i);
		rc = fs_unlink(path);
		zassert_equal(rc, 0, "unlink %u failed: %d", i, rc);
	}

	make_dir_path(path, sizeof(path), INDIRECT_FIRST);
	rc = fs_unlink(path);
	zassert_equal(rc, 0, "unlink of first indirect entry failed: %d", rc);

	zassert_equal(fs_unmount(mp), 0, "unmount failed");
	mp->flags = FS_MOUNT_FLAG_NO_FORMAT;
	zassert_equal(fs_mount(mp), 0, "remount failed");

	make_dir_path(path, sizeof(path), SURVIVOR);
	rc = fs_stat(path, &stat);
	zassert_equal(rc, 0, "survivor stat failed after unlink: %d", rc);

	zassert_equal(fs_unmount(mp), 0, "final unmount failed");
}
