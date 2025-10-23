/*
 * Copyright (c) 2024 Linumiz
 * Copyright (c) 2026 Antmicro <www.antmicro.com>
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

#define FATFS_MNTP "/RAM:"
#define TEST_FILE  FATFS_MNTP "/testfile.txt"

static FATFS fat_fs;
static FILE *file;

static char testchar = 'a';
static const char teststr[5] = "bcdef"; /* no null-termination */
#define FINAL_CONTENT_LENGTH (sizeof(testchar) + sizeof(teststr))

static struct fs_mount_t fatfs_mnt = {
	.type = FS_FATFS,
	.mnt_point = FATFS_MNTP,
	.fs_data = &fat_fs,
};

void create_file(void *fixture)
{
	ARG_UNUSED(fixture);
	zassert_ok(fs_mount(&fatfs_mnt), "Error mounting file system\n");
	file = fopen(TEST_FILE, "w+x");
	zassert_not_null(file, "Error creating file: %d\n", errno);
}

void close_file(void *fixture)
{
	ARG_UNUSED(fixture);
	zassert_ok(fclose(file), "Error closing file\n");
	zassert_ok(remove(TEST_FILE), "Error removing file: %d\n", errno);
	zassert_ok(fs_unmount(&fatfs_mnt), "Error unmounting file system\n");
}

ZTEST_SUITE(libc_stdio, NULL, NULL, create_file, close_file, NULL);

ZTEST(libc_stdio, test_fileobj_fputc)
{
	zassert_equal(testchar, fputc(testchar, file), "Error writing single character: %d\n",
		      errno);
}

ZTEST(libc_stdio, test_fileobj_fwrite)
{
	zassert_equal(sizeof(teststr),
		      fwrite(&teststr, sizeof(teststr[0]), ARRAY_SIZE(teststr), file),
		      "Error writing to file: %d\n", errno);
}

ZTEST(libc_stdio, test_fileobj_ftell)
{
	zassert_equal(0, ftell(file));
	zassert_equal(sizeof(teststr),
		      fwrite(&teststr, sizeof(teststr[0]), ARRAY_SIZE(teststr), file),
		      "Error writing to file: %d\n", errno);
	zassert_equal(ARRAY_SIZE(teststr), ftell(file));
}

ZTEST(libc_stdio, test_fileobj_fseek)
{
	zassert_ok(fseek(file, 0, SEEK_SET), "Error seeking: %d\n", errno);
}

ZTEST(libc_stdio, test_fileobj_fread)
{
	zassert_equal(testchar, fputc(testchar, file), "Error writing single character: %d\n",
		      errno);
	zassert_equal(sizeof(teststr),
		      fwrite(&teststr, sizeof(teststr[0]), ARRAY_SIZE(teststr), file),
		      "Error writing to file: %d\n", errno);
	zassert_ok(fseek(file, 0, SEEK_SET), "Error seeking: %d\n", errno);
	char rdbuf[FINAL_CONTENT_LENGTH] = {0};

	zassert_equal(sizeof(rdbuf), fread(&rdbuf, sizeof(rdbuf[0]), ARRAY_SIZE(rdbuf), file),
		      "Error reading from file: %d\n", errno);
	zassert_equal(rdbuf[0], testchar, "Read incorrect");
	zassert_mem_equal(&rdbuf[1], &teststr, sizeof(teststr), "Read incorrect");
}

ZTEST(libc_stdio, test_remove)
{
	zassert_equal(remove(""), -1, "Error Invalid path removed\n");
	zassert_equal(remove(NULL), -1, "Error Invalid path removed\n");
}
