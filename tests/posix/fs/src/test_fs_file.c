/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <fcntl.h>
#include <zephyr/posix/unistd.h>
#include "test_fs.h"

const char test_str[] = "hello world!";
int file = -1;

static int test_file_open(void)
{
	int res;

	res = open(TEST_FILE, O_CREAT | O_RDWR);
	if (res < 0) {
		TC_ERROR("Failed opening file: %d, errno=%d\n", res, errno);
		/* FIXME: restructure tests as per #46897 */
		__ASSERT_NO_MSG(res >= 0);
	}

	file = res;

	return TC_PASS;
}

int test_file_write(void)
{
	ssize_t brw;
	off_t res;

	res = lseek(file, 0, SEEK_SET);
	if (res != 0) {
		TC_PRINT("lseek failed [%d]\n", (int)res);
		close(file);
		file = -1;
		return TC_FAIL;
	}

	brw = write(file, (char *)test_str, strlen(test_str));
	if (brw < 0) {
		TC_PRINT("Failed writing to file [%d]\n", (int)brw);
		close(file);
		file = -1;
		return TC_FAIL;
	}

	if (brw < strlen(test_str)) {
		TC_PRINT("Unable to complete write. Volume full.\n");
		TC_PRINT("Number of bytes written: [%d]\n", (int)brw);
		close(file);
		file = -1;
		return TC_FAIL;
	}

	return res;
}

static int test_file_read(void)
{
	ssize_t brw;
	off_t res;
	char read_buff[80];
	size_t sz = strlen(test_str);

	res = lseek(file, 0, SEEK_SET);
	if (res != 0) {
		TC_PRINT("lseek failed [%d]\n", (int)res);
		close(file);
		file = -1;
		return TC_FAIL;
	}

	brw = read(file, read_buff, sz);
	if (brw < 0) {
		TC_PRINT("Failed reading file [%d]\n", (int)brw);
		close(file);
		file = -1;
		return TC_FAIL;
	}

	read_buff[brw] = 0;

	if (strcmp(test_str, read_buff)) {
		TC_PRINT("Error - Data read does not match data written\n");
		TC_PRINT("Data read:\"%s\"\n\n", read_buff);
		return TC_FAIL;
	}

	/* Now test after non-zero lseek. */

	res = lseek(file, 2, SEEK_SET);
	if (res != 2) {
		TC_PRINT("lseek failed [%d]\n", (int)res);
		close(file);
		file = -1;
		return TC_FAIL;
	}

	brw = read(file, read_buff, sizeof(read_buff));
	if (brw < 0) {
		TC_PRINT("Failed reading file [%d]\n", (int)brw);
		close(file);
		file = -1;
		return TC_FAIL;
	}

	/* Check for array overrun */
	brw = (brw < 80) ? brw : brw - 1;

	read_buff[brw] = 0;

	if (strcmp(test_str + 2, read_buff)) {
		TC_PRINT("Error - Data read does not match data written\n");
		TC_PRINT("Data read:\"%s\"\n\n", read_buff);
		return TC_FAIL;
	}

	return TC_PASS;
}

static int test_file_close(void)
{
	int res = 0;

	if (file >= 0) {
		res = close(file);
		if (res < 0) {
			TC_ERROR("Failed closing file: %d, errno=%d\n", res, errno);
			/* FIXME: restructure tests as per #46897 */
			__ASSERT_NO_MSG(res == 0);
		}

		file = -1;
	}

	return res;
}

static int test_file_delete(void)
{
	int res;

	res = unlink(TEST_FILE);
	if (res) {
		TC_PRINT("Error deleting file [%d]\n", res);
		return res;
	}

	return res;
}

static void after_fn(void *unused)
{
	ARG_UNUSED(unused);

	test_file_close();
	unlink(TEST_FILE);
}

ZTEST_SUITE(posix_fs_file_test, NULL, test_mount, NULL, after_fn,
	    test_unmount);

/**
 * @brief Test for POSIX open API
 *
 * @details Test opens new file through POSIX open API.
 */
ZTEST(posix_fs_file_test, test_fs_open)
{
	/* FIXME: restructure tests as per #46897 */
	zassert_true(test_file_open() == TC_PASS, NULL);
}

/**
 * @brief Test for POSIX write API
 *
 * @details Test writes some data through POSIX write API.
 */
ZTEST(posix_fs_file_test, test_fs_write)
{
	/* FIXME: restructure tests as per #46897 */
	zassert_true(test_file_open() == TC_PASS, NULL);
	zassert_true(test_file_write() == TC_PASS, NULL);
}

/**
 * @brief Test for POSIX write API
 *
 * @details Test reads data back through POSIX read API.
 */
ZTEST(posix_fs_file_test, test_fs_read)
{
	/* FIXME: restructure tests as per #46897 */
	zassert_true(test_file_open() == TC_PASS, NULL);
	zassert_true(test_file_write() == TC_PASS, NULL);
	zassert_true(test_file_read() == TC_PASS, NULL);
}

/**
 * @brief Test for POSIX close API
 *
 * @details Test closes the open file through POSIX close API.
 */
ZTEST(posix_fs_file_test, test_fs_close)
{
	/* FIXME: restructure tests as per #46897 */
	zassert_true(test_file_open() == TC_PASS, NULL);
	zassert_true(test_file_close() == TC_PASS, NULL);
}

/**
 * @brief Test for POSIX unlink API
 *
 * @details Test deletes a file through POSIX unlink API.
 */
ZTEST(posix_fs_file_test, test_fs_unlink)
{
	zassert_true(test_file_open() == TC_PASS, NULL);
	zassert_true(test_file_delete() == TC_PASS, NULL);
}

ZTEST(posix_fs_file_test, test_fs_fd_leak)
{
	const int reps =
	    MAX(CONFIG_POSIX_MAX_OPEN_FILES, CONFIG_POSIX_MAX_FDS) + 5;

	for (int i = 0; i < reps; i++) {
		if (i > 0) {
			zassert_true(test_file_open() == TC_PASS, NULL);
		}

		if (i < reps - 1) {
			zassert_true(test_file_close() == TC_PASS, NULL);
		}
	}
}
