/*
 * Copyright (c) 2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>

/* Mount point and paths must be provided by test runner. */
extern struct fs_mount_t *fs_mkfs_mp;
extern const int fs_mkfs_type;
extern uintptr_t fs_mkfs_dev_id;
extern int fs_mkfs_flags;
extern const char *some_file_path;
extern const char *other_dir_path;

/* This test assumes that storage device is erased. */
void test_fs_mkfs_simple(void)
{
	int ret = 0;

	TC_PRINT("Mount with flag no format\n");
	fs_mkfs_mp->flags = FS_MOUNT_FLAG_NO_FORMAT;
	ret = fs_mount(fs_mkfs_mp);

	if (fs_mkfs_mp->type == FS_LITTLEFS) {
		zassert_equal(ret, -EILSEQ, "Expected EILSEQ got %d", ret);
	} else if (fs_mkfs_mp->type == FS_FATFS) {
		zassert_equal(ret, -ENODEV, "Expected ENODEV got %d", ret);
	}

	TC_PRINT("Try mkfs with RW access\n");
	ret = fs_mkfs(fs_mkfs_type, fs_mkfs_dev_id, NULL, fs_mkfs_dev_id);
	zassert_equal(ret, 0, "Expected successful mkfs (%d)", ret);

	TC_PRINT("Mount created file system without formatting\n");
	fs_mkfs_mp->flags = FS_MOUNT_FLAG_NO_FORMAT;
	ret = fs_mount(fs_mkfs_mp);
	zassert_equal(ret, 0, "Expected successful mount (%d)", ret);
	ret = fs_unmount(fs_mkfs_mp);
	zassert_equal(ret, 0, "Expected fs_unmount success (%d)", ret);
}

/* This test assumes that storage device is erased. */
void test_fs_mkfs_ops(void)
{
	int ret = 0;
	struct fs_file_t fs;
	struct fs_dir_t dir;
	struct fs_dirent entry;

	fs_file_t_init(&fs);
	fs_dir_t_init(&dir);

	/* Format volume and add some files/dirs */
	TC_PRINT("Mount to prepare tests\n");
	fs_mkfs_mp->flags = 0;
	ret = fs_mount(fs_mkfs_mp);
	zassert_equal(ret, 0, "Expected success (%d)", ret);
	TC_PRINT("Create some file\n");
	ret = fs_open(&fs, some_file_path, FS_O_CREATE);
	zassert_equal(ret, 0, "Expected success fs_open(FS_O_CREATE) (%d)",
		      ret);
	ret = fs_close(&fs);
	zassert_equal(ret, 0, "Expected fs_close success (%d)", ret);
	TC_PRINT("Create other directory\n");
	ret = fs_mkdir(other_dir_path);
	zassert_equal(ret, 0, "Expected fs_mkdir success (%d)", ret);
	ret = fs_unmount(fs_mkfs_mp);
	zassert_equal(ret, 0, "Expected fs_umount success (%d)", ret);

	/* Format file system using fs_mkfs and check if it was formatted */
	TC_PRINT("Try mkfs\n");
	ret = fs_mkfs(fs_mkfs_type, fs_mkfs_dev_id, NULL, fs_mkfs_dev_id);
	zassert_equal(ret, 0, "Expected successful format (%d)", ret);
	fs_mkfs_mp->flags = FS_MOUNT_FLAG_NO_FORMAT;
	ret = fs_mount(fs_mkfs_mp);
	zassert_equal(ret, 0, "Expected successful mount (%d)", ret);
	ret = fs_stat(some_file_path, &entry);
	zassert_equal(ret, -ENOENT, "Expected ENOENT got %d", ret);
	ret = fs_stat(other_dir_path, &entry);
	zassert_equal(ret, -ENOENT, "Expected ENOENT got %d", ret);
	TC_PRINT("Create some file\n");
	ret = fs_open(&fs, some_file_path, FS_O_CREATE);
	zassert_equal(ret, 0, "Expected success fs_open(FS_O_CREATE) (%d)",
		      ret);
	ret = fs_close(&fs);
	zassert_equal(ret, 0, "Expected fs_close success (%d)", ret);
	TC_PRINT("Create other directory\n");
	ret = fs_mkdir(other_dir_path);
	zassert_equal(ret, 0, "Expected fs_mkdir success (%d)", ret);
	ret = fs_unmount(fs_mkfs_mp);
	zassert_equal(ret, 0, "Expected fs_unmount success (%d)", ret);
}
