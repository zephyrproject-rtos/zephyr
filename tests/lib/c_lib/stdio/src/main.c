/*
 * Copyright (c) 2024 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>
#include <ff.h>
#include <zephyr/fs/fs.h>

#define FATFS_MNTP	"/RAM:"
#define TEST_FILE	FATFS_MNTP"/testfile.txt"

static FATFS fat_fs;

static struct fs_mount_t fatfs_mnt = {
	.type = FS_FATFS,
	.mnt_point = FATFS_MNTP,
	.fs_data = &fat_fs,
};

ZTEST_SUITE(libc_stdio, NULL, NULL, NULL, NULL, NULL);

/**
 * @brief Test for remove API
 *
 * @details Test deletes a file through remove API.
 */
ZTEST(libc_stdio, test_remove)
{
	struct fs_file_t file;

	fs_file_t_init(&file);

	zassert_ok(fs_mount(&fatfs_mnt), "Error in mount file system\n");
	zassert_equal(fs_open(&file, TEST_FILE, FS_O_CREATE), 0,
		      "Error creating file\n");
	zassert_ok(fs_close(&file), "Error closing file\n");
	zassert_ok(remove(TEST_FILE), "Error removing file: %d\n", errno);
	zassert_equal(remove(""), -1, "Error Invalid path removed\n");
	zassert_equal(remove(NULL), -1, "Error Invalid path removed\n");
	zassert_ok(fs_unmount(&fatfs_mnt), "Error while unmount file system\n");
}
