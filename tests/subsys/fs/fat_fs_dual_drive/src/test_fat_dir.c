/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @filesystem
 * @brief test_filesystem
 * Demonstrates the ZEPHYR File System APIs
 */

#include "test_fat.h"
#include <stdio.h>

extern int test_file_write(void);
extern int test_file_close(void);
static int test_rmdir(const char *dir);

static int test_mkdir(const char *dir, const char *file)
{
	int res;

	TC_PRINT("mkdir tests:\n");

	if (check_file_dir_exists(dir)) {
		TC_PRINT("[%s] exists, delete it\n", dir);
		if (test_rmdir(dir)) {
			TC_PRINT("Error deleting dir %s\n", dir);
			return TC_FAIL;
		}
	} else {
		TC_PRINT("Creating new dir %s\n", dir);
	}

	/* Verify fs_mkdir() */
	res = fs_mkdir(dir);
	if (res) {
		TC_PRINT("Error creating dir[%d]\n", res);
		return res;
	}

	res = fs_open(&filep, file, FS_O_CREATE | FS_O_RDWR);
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

	TC_PRINT("Created dir %s!\n", dir);

	return res;
}

static int test_lsdir(const char *path)
{
	int res;
	struct fs_dir_t dirp;
	struct fs_dirent entry;

	TC_PRINT("lsdir tests:\n");

	/* Verify fs_opendir() */
	res = fs_opendir(&dirp, path);
	if (res) {
		TC_PRINT("Error opening dir %s [%d]\n", path, res);
		return res;
	}

	TC_PRINT("Listing dir %s:\n", path);
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

static int test_rmdir(const char *dir)
{
	int res;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;
	char file_path[80];

	TC_PRINT("rmdir tests:\n");

	if (!check_file_dir_exists(dir)) {
		TC_PRINT("%s doesn't exist\n", dir);
		return TC_FAIL;
	}

	res = fs_opendir(&dirp, dir);
	if (res) {
		TC_PRINT("Error opening dir[%d]\n", res);
		return res;
	}

	TC_PRINT("Removing files and sub directories in %s\n", dir);
	for (;;) {
		res = fs_readdir(&dirp, &entry);

		/* entry.name[0] == 0 means end-of-dir */
		if (res || entry.name[0] == 0) {
			break;
		}

		/* Delete file or sub directory */
		sprintf(file_path, "%s/%s", dir, entry.name);
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
	res = fs_unlink(dir);
	if (res) {
		TC_PRINT("Error removing dir [%d]\n", res);
		return res;
	}

	TC_PRINT("Removed dir %s!\n", dir);

	return res;
}

void test_fat_dir(void)
{
	TC_PRINT("\nTesting directory operations on %s\n", FATFS_MNTP);
	zassert_true(test_mkdir(TEST_DIR, TEST_DIR_FILE) == TC_PASS, NULL);
	zassert_true(test_lsdir(FATFS_MNTP) == TC_PASS, NULL);
	zassert_true(test_lsdir(TEST_DIR) == TC_PASS, NULL);
	zassert_true(test_rmdir(TEST_DIR) == TC_PASS, NULL);
	zassert_true(test_lsdir(FATFS_MNTP) == TC_PASS, NULL);

	TC_PRINT("\nTesting directory operations on %s\n", FATFS_MNTP1);
	zassert_true(test_mkdir(TEST_DIR1, TEST_DIR_FILE1) == TC_PASS, NULL);
	zassert_true(test_lsdir(FATFS_MNTP1) == TC_PASS, NULL);
	zassert_true(test_lsdir(TEST_DIR1) == TC_PASS, NULL);
	zassert_true(test_rmdir(TEST_DIR1) == TC_PASS, NULL);
	zassert_true(test_lsdir(FATFS_MNTP1) == TC_PASS, NULL);
}
