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

ZTEST(libc_stdio, test_remove)
{
	struct fs_file_t file;

	/* first create the file, using native calls */
	fs_file_t_init(&file);
	zassert_equal(fs_open(&file, TEST_FILE, FS_O_CREATE), 0,
		      "Error creating file\n");
	zassert_ok(fs_close(&file), "Error closing file\n");

	/* then exercise remove() */
	zassert_ok(remove(TEST_FILE), "Error removing file: %d\n", errno);
	zassert_equal(remove(""), -1, "Error Invalid path removed\n");
}

#if defined(CONFIG_COMMON_LIBC_FOPEN)
ZTEST(libc_stdio, test_fopen_fclose)
{
	FILE *fp;

	fp = fopen(TEST_FILE, "r");
	zassert_is_null(fp, "File unexpectedly opened\n");

	fp = fopen(TEST_FILE, "w");
	zassert_not_null(fp, "Error opening file\n");
	zassert_ok(fclose(fp), "Error closing file\n");
}

ZTEST(libc_stdio, test_freopen)
{
	FILE *fp;
	const char *reopen_data = "reopen test\n";

	fp = fopen(TEST_FILE, "w");
	zassert_not_null(fp, "Error opening file for write\n");
	zassert_ok(fputs(reopen_data, fp), "Error writing file\n");
	zassert_ok(fclose(fp), "Error closing file after write\n");

	fp = fopen(TEST_FILE, "r");
	zassert_not_null(fp, "Error opening file for read\n");
	zassert_ok(fclose(fp), "Error closing file after read\n");

	fp = fopen(TEST_FILE, "w");
	zassert_not_null(fp, "Error opening file before freopen\n");
	fp = freopen(TEST_FILE, "r", fp);
	zassert_not_null(fp, "Error reopening file for read\n");
	zassert_ok(fclose(fp), "Error closing file after freopen\n");
}
#endif /* CONFIG_COMMON_LIBC_FOPEN */

static void before(void *arg)
{
	ARG_UNUSED(arg);

	zassert_ok(fs_mount(&fatfs_mnt), "Error in mount file system\n");
}

static void after(void *arg)
{
	ARG_UNUSED(arg);

	(void)fs_unlink(TEST_FILE);
	zassert_ok(fs_unmount(&fatfs_mnt), "Error while unmount file system\n");
}

ZTEST_SUITE(libc_stdio, NULL, NULL, before, after, NULL);
