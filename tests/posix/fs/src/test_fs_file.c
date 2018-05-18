/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <posix/unistd.h>
#include "test_fs.h"

const char test_str[] = "hello world!";
int file;

static int test_file_open(void)
{
	int res;

	TC_PRINT("\nOpen tests:\n");

	res = open(TEST_FILE, O_RDWR);
	if (res < 0) {
		TC_PRINT("Failed opening file [%d]\n", res);
		return res;
	}

	file = res;

	TC_PRINT("Opened file %s\n", TEST_FILE);

	return 0;
}

int test_file_write(void)
{
	int brw;
	int res;

	TC_PRINT("\nWrite tests:\n");

	res = lseek(file, 0, SEEK_SET);
	if (res) {
		TC_PRINT("lseek failed [%d]\n", res);
		close(file);
		return res;
	}

	TC_PRINT("Data written:\"%s\"\n\n", test_str);

	brw = write(file, (char *)test_str, strlen(test_str));
	if (brw < 0) {
		TC_PRINT("Failed writing to file [%d]\n", brw);
		close(file);
		return brw;
	}

	if (brw < strlen(test_str)) {
		TC_PRINT("Unable to complete write. Volume full.\n");
		TC_PRINT("Number of bytes written: [%d]\n", brw);
		close(file);
		return TC_FAIL;
	}

	TC_PRINT("Data successfully written!\n");

	return res;
}

static int test_file_read(void)
{
	int brw;
	int res;
	char read_buff[80];
	size_t sz = strlen(test_str);

	TC_PRINT("\nRead tests:\n");

	res = lseek(file, 0, SEEK_SET);
	if (res) {
		TC_PRINT("lseek failed [%d]\n", res);
		close(file);
		return res;
	}

	brw = read(file, read_buff, sz);
	if (brw < 0) {
		TC_PRINT("Failed reading file [%d]\n", brw);
		close(file);
		return brw;
	}

	read_buff[brw] = 0;

	TC_PRINT("Data read:\"%s\"\n\n", read_buff);

	if (strcmp(test_str, read_buff)) {
		TC_PRINT("Error - Data read does not match data written\n");
		TC_PRINT("Data read:\"%s\"\n\n", read_buff);
		return TC_FAIL;
	}

	TC_PRINT("Data read matches data written\n");

	return res;
}

static int test_file_close(void)
{
	int res;

	TC_PRINT("\nClose tests:\n");

	res = close(file);
	if (res) {
		TC_PRINT("Error closing file [%d]\n", res);
		return res;
	}

	TC_PRINT("Closed file %s\n", TEST_FILE);

	return res;
}

static int test_file_delete(void)
{
	int res;

	TC_PRINT("\nDelete tests:\n");

	res = unlink(TEST_FILE);
	if (res) {
		TC_PRINT("Error deleting file [%d]\n", res);
		return res;
	}

	TC_PRINT("File (%s) deleted successfully!\n", TEST_FILE);

	return res;
}

/**
 * @brief Test for POSIX open API
 *
 * @details Test opens new file through POSIX open API.
 */
void test_fs_open(void)
{
	zassert_true(test_file_open() == TC_PASS, NULL);
}

/**
 * @brief Test for POSIX write API
 *
 * @details Test writes some data through POSIX write API.
 */
void test_fs_write(void)
{
	zassert_true(test_file_write() == TC_PASS, NULL);
}

/**
 * @brief Test for POSIX write API
 *
 * @details Test reads data back through POSIX read API.
 */
void test_fs_read(void)
{
	zassert_true(test_file_read() == TC_PASS, NULL);
}

/**
 * @brief Test for POSIX close API
 *
 * @details Test closes the open file through POSIX close API.
 */
void test_fs_close(void)
{
	zassert_true(test_file_close() == TC_PASS, NULL);
}

/**
 * @brief Test for POSIX unlink API
 *
 * @details Test deletes a file through POSIX unlink API.
 */
void test_fs_unlink(void)
{
	zassert_true(test_file_delete() == TC_PASS, NULL);
}
