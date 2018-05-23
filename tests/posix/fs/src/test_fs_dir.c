/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <posix/unistd.h>
#include "test_fs.h"

extern int test_file_write(void);
extern int test_file_close(void);
extern int file;

static int test_mkdir(void)
{
	int res;

	TC_PRINT("\nmkdir tests:\n");

	/* Verify fs_mkdir() */
	res = mkdir(TEST_DIR, S_IRWXG);
	if (res) {
		TC_PRINT("Error creating dir[%d]\n", res);
		return res;
	}

	res = open(TEST_DIR_FILE, O_RDWR);
	if (res < 0) {
		TC_PRINT("Failed opening file [%d]\n", res);
		return res;
	}
	file = res;

	res = test_file_write();
	if (res) {
		return res;
	}

	res = close(file);
	if (res) {
		TC_PRINT("Error closing file [%d]\n", res);
		return res;
	}

	TC_PRINT("Created dir %s!\n", TEST_DIR);

	return res;
}

static int test_lsdir(const char *path)
{
	DIR *dirp;
	struct dirent *entry;

	TC_PRINT("\nreaddir test:\n");

	/* Verify fs_opendir() */
	dirp = opendir(path);
	if (dirp == NULL) {
		TC_PRINT("Error opening dir %s\n", path);
		return -EIO;
	}

	TC_PRINT("\nListing dir %s:\n", path);
	/* Verify fs_readdir() */
	entry = readdir(dirp);

	/* entry.name[0] == 0 means end-of-dir */
	if (entry == NULL) {
		closedir(dirp);
		return -EIO;
	} else {
		TC_PRINT("[FILE] %s\n", entry->d_name);
	}

	/* Verify fs_closedir() */
	closedir(dirp);

	return 0;
}

/**
 * @brief Test for POSIX mkdir API
 *
 * @details Test creates a new directory through POSIX
 * mkdir API and open a new file under the directory and
 * writes some data into the file.
 */
void test_fs_mkdir(void)
{
	zassert_true(test_mkdir() == TC_PASS, NULL);
}

/**
 * @brief Test for POSIX opendir, readdir and closedir API
 *
 * @details Test opens an existing directory through POSIX
 * opendir API, reads the contents of the directory through
 * readdir API and closes it through closedir API.
 */
void test_fs_readdir(void)
{
	zassert_true(test_lsdir(TEST_DIR) == TC_PASS, NULL);
}
