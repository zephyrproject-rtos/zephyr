/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_fat.h"
#include <string.h>

static int test_file_open(void)
{
	int res;

	TC_PRINT("\nOpen tests:\n");

	if (check_file_dir_exists(TEST_FILE)) {
		TC_PRINT("Opening existing file %s\n", TEST_FILE);
	} else {
		TC_PRINT("Creating new file %s\n", TEST_FILE);
	}

	/* Verify fs_open() */
	res = fs_open(&filep, TEST_FILE);
	if (res) {
		TC_PRINT("Failed opening file [%d]\n", res);
		return res;
	}

	TC_PRINT("Opened file %s\n", TEST_FILE);

	return res;
}

int test_file_write(void)
{
	ssize_t brw;
	int res;

	TC_PRINT("\nWrite tests:\n");

	/* Verify fs_seek() */
	res = fs_seek(&filep, 0, FS_SEEK_SET);
	if (res) {
		TC_PRINT("fs_seek failed [%d]\n", res);
		fs_close(&filep);
		return res;
	}

	TC_PRINT("Data written:\"%s\"\n\n", test_str);

	/* Verify fs_write() */
	brw = fs_write(&filep, (char *)test_str, strlen(test_str));
	if (brw < 0) {
		TC_PRINT("Failed writing to file [%zd]\n", brw);
		fs_close(&filep);
		return brw;
	}

	if (brw < strlen(test_str)) {
		TC_PRINT("Unable to complete write. Volume full.\n");
		TC_PRINT("Number of bytes written: [%zd]\n", brw);
		fs_close(&filep);
		return TC_FAIL;
	}

	TC_PRINT("Data successfully written!\n");

	return res;
}

static int test_file_read(void)
{
	ssize_t brw;
	int res;
	char read_buff[80];
	size_t sz = strlen(test_str);

	TC_PRINT("\nRead tests:\n");

	res = fs_seek(&filep, 0, FS_SEEK_SET);
	if (res) {
		TC_PRINT("fs_seek failed [%d]\n", res);
		fs_close(&filep);
		return res;
	}

	/* Verify fs_read() */
	brw = fs_read(&filep, read_buff, sz);
	if (brw < 0) {
		TC_PRINT("Failed reading file [%zd]\n", brw);
		fs_close(&filep);
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

	res = fs_close(&filep);
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

	/* Verify fs_unlink() */
	res = fs_unlink(TEST_FILE);
	if (res) {
		TC_PRINT("Error deleting file [%d]\n", res);
		return res;
	}

	/* Check if file was deleted */
	if (check_file_dir_exists(TEST_FILE)) {
		TC_PRINT("Failed deleting %s\n", TEST_FILE);
		return TC_FAIL;
	}

	TC_PRINT("File (%s) deleted successfully!\n", TEST_FILE);

	return res;
}

void test_fat_open(void)
{
	zassert_true(test_file_open() == TC_PASS, NULL);
}

void test_fat_write(void)
{
	zassert_true(test_file_write() == TC_PASS, NULL);
}

void test_fat_read(void)
{
	zassert_true(test_file_read() == TC_PASS, NULL);
}

void test_fat_close(void)
{
	zassert_true(test_file_close() == TC_PASS, NULL);
}
void test_fat_unlink(void)
{
	zassert_true(test_file_delete() == TC_PASS, NULL);
}
