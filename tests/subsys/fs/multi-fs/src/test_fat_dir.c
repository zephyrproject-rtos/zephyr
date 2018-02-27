/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_fat.h"
#include <stdio.h>

extern int test_file_write(void);
extern int test_file_close(void);
static int test_rmdir(void);

static int test_mkdir(void)
{
	int res;

	TC_PRINT("\nmkdir tests:\n");

	if (check_file_dir_exists(TEST_DIR)) {
		TC_PRINT("[%s] exists, delete it\n", TEST_DIR);
		if (test_rmdir()) {
			TC_PRINT("Error deleting dir %s\n", TEST_DIR);
			return TC_FAIL;
		}
	} else {
		TC_PRINT("Creating new dir %s\n", TEST_DIR);
	}

	/* Verify fs_mkdir() */
	res = fs_mkdir(TEST_DIR);
	if (res) {
		TC_PRINT("Error creating dir[%d]\n", res);
		return res;
	}

	res = fs_open(&filep, TEST_DIR_FILE);
	if (res) {
		TC_PRINT("Failed opening file [%d]\n", res);
		return res;
	}

	res = test_file_write();
	if (res) {
		return res;
	}

	res = fs_close(&filep);
	if (res) {
		TC_PRINT("Error closing file [%d]\n", res);
		return res;
	}

	TC_PRINT("Created dir %s!\n", TEST_DIR);

	return res;
}

static int test_lsdir(const char *path)
{
	int res;
	struct fs_dir_t dirp;
	struct fs_dirent entry;

	TC_PRINT("\nlsdir tests:\n");

	/* Verify fs_opendir() */
	res = fs_opendir(&dirp, path);
	if (res) {
		TC_PRINT("Error opening dir %s [%d]\n", path, res);
		return res;
	}

	TC_PRINT("\nListing dir %s:\n", path);
	for (;;) {
		/* Verify fs_readdir() */
		res = fs_readdir(&dirp, &entry);

		/* entry.name[0] == 0 means end-of-dir */
		if (res || entry.name[0] == 0) {
			break;
		}

		if (entry.type == FS_DIR_ENTRY_DIR) {
			TC_PRINT("[DIR ] %s\n", entry.name);
		} else {
			TC_PRINT("[FILE] %s (size = %zu)\n",
				entry.name, entry.size);
		}
	}

	/* Verify fs_closedir() */
	fs_closedir(&dirp);

	return res;
}

static int test_rmdir(void)
{
	int res;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;
	char file_path[80];

	TC_PRINT("\nrmdir tests:\n");

	if (!check_file_dir_exists(TEST_DIR)) {
		TC_PRINT("%s doesn't exist\n", TEST_DIR);
		return TC_FAIL;
	}

	res = fs_opendir(&dirp, TEST_DIR);
	if (res) {
		TC_PRINT("Error opening dir[%d]\n", res);
		return res;
	}

	TC_PRINT("\nRemoving files and sub directories in %s\n", TEST_DIR);
	for (;;) {
		res = fs_readdir(&dirp, &entry);

		/* entry.name[0] == 0 means end-of-dir */
		if (res || entry.name[0] == 0) {
			break;
		}

		/* Delete file or sub directory */
		sprintf(file_path, "%s/%s", TEST_DIR, entry.name);
		TC_PRINT("Removing %s\n", file_path);

		res = fs_unlink(file_path);
		if (res) {
			TC_PRINT("Error deleting file/dir [%d]\n", res);
			fs_closedir(&dirp);
			return res;
		}
	}

	fs_closedir(&dirp);

	/* Verify fs_unlink() */
	res = fs_unlink(TEST_DIR);
	if (res) {
		TC_PRINT("Error removing dir [%d]\n", res);
		return res;
	}

	TC_PRINT("Removed dir %s!\n", TEST_DIR);

	return res;
}

void test_fat_mkdir(void)
{
	zassert_true(test_mkdir() == TC_PASS, NULL);
}

void test_fat_readdir(void)
{
	zassert_true(test_lsdir(FATFS_MNTP) == TC_PASS, NULL);
}

void test_fat_rmdir(void)
{
	zassert_true(test_rmdir() == TC_PASS, NULL);
}
