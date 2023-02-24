/*
 * Copyright (c) 2023 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>

#include "utils.h"

ZTEST(ext2tests, test_dirops_basic)
{
	struct fs_mount_t *mp = &testfs_mnt;

	zassert_equal(fs_mount(mp), 0, "Mount failed");

	struct fs_file_t file;

	fs_file_t_init(&file);

	/* Create some directories */
	zassert_equal(fs_mkdir("/sml/dir1"), 0, "Create dir1 failed");
	zassert_equal(fs_mkdir("/sml/dir2"), 0, "Create dir2 failed");

	/* Create some files */
	zassert_equal(fs_open(&file, "/sml/file1", FS_O_CREATE), 0, "Create file1 failed");
	zassert_equal(fs_close(&file), 0, "Close file error");
	zassert_equal(fs_open(&file, "/sml/dir1/file2", FS_O_CREATE), 0, "Create file2 failed");
	zassert_equal(fs_close(&file), 0, "Close file error");
	zassert_equal(fs_open(&file, "/sml/dir2/file3", FS_O_CREATE), 0, "Create file3 failed");
	zassert_equal(fs_close(&file), 0, "Close file error");

	/* Check if directories will open as files */
	zassert_equal(fs_open(&file, "/sml/dir1", 0), -EINVAL, "Should return error");
	zassert_equal(fs_open(&file, "/sml/dir2", 0), -EINVAL, "Should return error");

	/* Check if files will open correctly */
	zassert_equal(fs_open(&file, "/sml/file1", 0), 0, "Open file1 should succeed");
	zassert_equal(fs_close(&file), 0, "Close file error");
	zassert_equal(fs_open(&file, "/sml/dir1/file2", 0), 0, "Open file2 should succeed");
	zassert_equal(fs_close(&file), 0, "Close file error");
	zassert_equal(fs_open(&file, "/sml/dir2/file3", 0), 0, "Open file3 should succeed");
	zassert_equal(fs_close(&file), 0, "Close file error");

	/* Check for some nonexisting files */
	zassert_equal(fs_open(&file, "/sml/file2", 0), -ENOENT, "Should not exist");
	zassert_equal(fs_open(&file, "/sml/file3", 0), -ENOENT, "Should not exist");
	zassert_equal(fs_open(&file, "/sml/dir1/file1", 0), -ENOENT, "Should not exist");
	zassert_equal(fs_open(&file, "/sml/dir1/file3", 0), -ENOENT, "Should not exist");
	zassert_equal(fs_open(&file, "/sml/dir2/file1", 0), -ENOENT, "Should not exist");
	zassert_equal(fs_open(&file, "/sml/dir2/file2", 0), -ENOENT, "Should not exist");

	zassert_equal(fs_unmount(mp), 0, "Unmount failed");
}
