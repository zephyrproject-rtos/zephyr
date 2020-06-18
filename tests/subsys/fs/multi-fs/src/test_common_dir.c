/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <fs/fs.h>
#include "test_common.h"

int test_rmdir(const char *dir_path);

int test_mkdir(const char *dir_path, const char *file)
{
	int res;
	struct fs_file_t filep;
	char file_path[PATH_MAX] = { 0 };

	res = sprintf(file_path, "%s/%s", dir_path, file);
	assert(res < sizeof(file_path));

	if (check_file_dir_exists(dir_path)) {
		TC_PRINT("[%s] exists, delete it\n", dir_path);
		if (test_rmdir(dir_path)) {
			TC_PRINT("Error deleting dir %s\n", dir_path);
			return TC_FAIL;
		}
	} else {
		TC_PRINT("Creating new dir %s\n", dir_path);
	}

	/* Verify fs_mkdir() */
	res = fs_mkdir(dir_path);
	if (res) {
		TC_PRINT("Error creating dir[%d]\n", res);
		return res;
	}

	res = fs_open(&filep, file_path, FS_O_CREATE | FS_O_RDWR);
	if (res) {
		TC_PRINT("Failed opening file [%d]\n", res);
		return res;
	}

	TC_PRINT("Testing write to file %s\n", file_path);
	res = test_file_write(&filep, "NOTHING");
	if (res) {
		return res;
	}

	res = fs_close(&filep);
	if (res) {
		TC_PRINT("Error closing file [%d]\n", res);
		return res;
	}

	TC_PRINT("Created dir %s!\n", dir_path);

	return res;
}

int test_lsdir(const char *path)
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

int test_rmdir(const char *dir_path)
{
	int res;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;

	if (!check_file_dir_exists(dir_path)) {
		TC_PRINT("%s doesn't exist\n", dir_path);
		return TC_FAIL;
	}

	res = fs_opendir(&dirp, dir_path);
	if (res) {
		TC_PRINT("Error opening dir[%d]\n", res);
		return res;
	}

	TC_PRINT("\nRemoving files and sub directories in %s\n", dir_path);
	for (;;) {
		char file_path[PATH_MAX] = { 0 };

		res = fs_readdir(&dirp, &entry);

		/* entry.name[0] == 0 means end-of-dir */
		if (res || entry.name[0] == 0) {
			break;
		}

		/* Delete file or sub directory */
		sprintf(file_path, "%s/%s", dir_path, entry.name);
		assert(res < sizeof(file_path));
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
	res = fs_unlink(dir_path);
	if (res) {
		TC_PRINT("Error removing dir [%d]\n", res);
		return res;
	}

	TC_PRINT("Removed dir %s!\n", dir_path);

	return res;
}
