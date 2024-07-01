/*
 * Copyright (c) 2023 Nikhef
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/dirent.h>
#include "test_fs.h"

#define FILL_SIZE 128

#define TEST_FILE_SIZE     80
#define TEST_DIR_FILE_SIZE 1000

#define TEST_EMPTY_FILE FATFS_MNTP "/empty.dat"

static void create_file(const char *filename, uint32_t size)
{
	int fh;

	fh = open(filename, O_CREAT | O_WRONLY);
	zassert(fh >= 0, "Failed creating test file");

	uint8_t filling[FILL_SIZE];

	while (size > FILL_SIZE) {
		zassert_equal(FILL_SIZE, write(fh, filling, FILL_SIZE));
		size -= FILL_SIZE;
	}

	zassert_equal(size, write(fh, filling, size));

	zassert_ok(close(fh));
}

static void before_fn(void *unused)
{
	ARG_UNUSED(unused);

	zassert_ok(mkdir(TEST_DIR, 0070));

	create_file(TEST_FILE, TEST_FILE_SIZE);
	create_file(TEST_DIR_FILE, TEST_DIR_FILE_SIZE);
	create_file(TEST_EMPTY_FILE, 0);
}

static void after_fn(void *unused)
{
	ARG_UNUSED(unused);

	zassert_ok(unlink(TEST_FILE));
	zassert_ok(unlink(TEST_DIR_FILE));
	zassert_ok(unlink(TEST_DIR));
}

ZTEST_SUITE(posix_fs_stat_test, NULL, test_mount, before_fn, after_fn, test_unmount);

/**
 * @brief Test stat command on file
 *
 * @details Tests file in root, file in directroy, non-existing file and empty file.
 */
ZTEST(posix_fs_stat_test, test_fs_stat_file)
{
	struct stat buf;

	zassert_equal(0, stat(TEST_FILE, &buf));
	zassert_equal(TEST_FILE_SIZE, buf.st_size);
	zassert_equal(S_IFREG, buf.st_mode);

	zassert_equal(0, stat(TEST_DIR_FILE, &buf));
	zassert_equal(TEST_DIR_FILE_SIZE, buf.st_size);
	zassert_equal(S_IFREG, buf.st_mode);

	zassert_not_equal(0, stat(TEST_ROOT "foo.txt", &buf));
	zassert_not_equal(0, stat("", &buf));

	zassert_equal(0, stat(TEST_EMPTY_FILE, &buf));

	zassert_equal(0, buf.st_size);
	zassert_equal(S_IFREG, buf.st_mode);
}

/**
 * @brief Test stat command on dir
 *
 * @details Tests if we can retrieve stastics for a directory. This should only provide S_IFDIR
 *          return value.
 */
ZTEST(posix_fs_stat_test, test_fs_stat_dir)
{
	struct stat buf;

	zassert_equal(0, stat(TEST_DIR, &buf));

	zassert_equal(0, buf.st_size);
	zassert_equal(S_IFDIR, buf.st_mode);

	/* note: for posix compatibility should should actually work */
	zassert_not_equal(0, stat(TEST_ROOT, &buf));
}
