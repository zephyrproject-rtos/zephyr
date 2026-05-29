/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <fcntl.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/dirent.h>
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

	res = open(TEST_DIR_FILE, O_CREAT | O_RDWR, 0770);

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

static struct dirent *readdir_wrap(DIR *dirp, bool thread_safe)
{
	if (thread_safe) {
		/* cannot declare on stack otherwise this test fails for qemu_x86/atom */
		static struct dirent entry;
		struct dirent *result = NULL;

		zassert_ok(readdir_r(dirp, &entry, &result));

		return result;
	} else {
		return readdir(dirp);
	}
}

static int test_lsdir(const char *path, bool thread_safe)
{
	DIR *dirp;
	int res = 0;
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
	errno = 0;
	while ((entry = readdir_wrap(dirp, thread_safe)) != NULL) {
		if (entry->d_name[0] == 0) {
			res = -EIO;
			break;
		}

		TC_PRINT("[FILE] %s\n", entry->d_name);
	}

	if (errno) {
		res = -EIO;
	}

	/* Verify fs_closedir() */
	closedir(dirp);

	return res;
}

static void after_fn(void *unused)
{
	ARG_UNUSED(unused);

	unlink(TEST_DIR_FILE);
	unlink(TEST_DIR);
}

/* FIXME: restructure tests as per #46897 */
ZTEST_SUITE(posix_fs_dir_test, NULL, test_mount, NULL, after_fn,
	    test_unmount);

/**
 * @brief Test for POSIX mkdir API
 *
 * @details Test creates a new directory through POSIX
 * mkdir API and open a new file under the directory and
 * writes some data into the file.
 */
ZTEST(posix_fs_dir_test, test_fs_mkdir)
{
	/* FIXME: restructure tests as per #46897 */
	zassert_true(test_mkdir() == TC_PASS);
}

/**
 * @brief Test for POSIX opendir, readdir and closedir API
 *
 * @details Test opens an existing directory through POSIX
 * opendir API, reads the contents of the directory through
 * readdir API and closes it through closedir API.
 */
ZTEST(posix_fs_dir_test, test_fs_readdir)
{
	/* FIXME: restructure tests as per #46897 */
	zassert_true(test_mkdir() == TC_PASS);
	zassert_true(test_lsdir(TEST_DIR, false) == TC_PASS);
}

/**
 * Same test as `test_fs_readdir`, but use thread-safe `readdir_r()` function
 */
ZTEST(posix_fs_dir_test, test_fs_readdir_threadsafe)
{
	/* FIXME: restructure tests as per #46897 */
	zassert_true(test_mkdir() == TC_PASS);
	zassert_true(test_lsdir(TEST_DIR, true) == TC_PASS);
}

/**
 * @brief Test for POSIX rmdir API
 *
 * @details Test creates a new directory through POSIX
 * mkdir API and remove directory using rmdir.
 */
ZTEST(posix_fs_dir_test, test_fs_rmdir)
{
#define IRWXG	0070
	/* Create and remove empty directory */
	zassert_ok(mkdir(TEST_DIR, IRWXG), "Error creating dir: %d", errno);
	zassert_ok(rmdir(TEST_DIR), "Error removing dir: %d\n", errno);

	/* Create directory and open a file in the directory
	 * now removing the directory will fail, test will
	 * fail in removal of non empty directory
	 */
	zassert_ok(mkdir(TEST_DIR, IRWXG), "Error creating dir: %d", errno);
	zassert_not_equal(open(TEST_DIR_FILE, O_CREAT | O_RDWR), -1,
			  "Error creating file: %d", errno);
	zassert_not_ok(rmdir(TEST_DIR), "Error Non empty dir removed");
	zassert_not_ok(rmdir(""), "Error Invalid path removed");
	zassert_not_ok(rmdir(NULL), "Error Invalid path removed");
	zassert_not_ok(rmdir("TEST_DIR."), "Error Invalid path removed");
	zassert_not_ok(rmdir(TEST_FILE), "Error file removed");
}
