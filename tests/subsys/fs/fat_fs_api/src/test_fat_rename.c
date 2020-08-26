/*
 * Copyright (c) 2018 Pushpal Sidhu
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include "test_fat.h"

static int delete_it(const char *path, int quiet)
{
	int res = 0;
	if (check_file_dir_exists(path)) {
		res = fs_unlink(path);
		if (res && !quiet) {
			TC_PRINT("Couldn't delete %s [%d]\n", path, res);
		}
	}

	return res;
}

static int create_file(const char *path)
{
	struct fs_file_t fp;
	int res = 0;
	if (!check_file_dir_exists(path)) {
		res = fs_open(&fp, path, FS_O_CREATE | FS_O_RDWR);
		if (!res) {
			res = fs_close(&fp);
		} else {
			TC_PRINT("Couldn't open %s [%d]\n", path, res);
		}
	}

	return res;
}

static int create_dir(const char *path)
{
	int res = 0;
	if (!check_file_dir_exists(path)) {
		res = fs_mkdir(path);
		if (res) {
			TC_PRINT("Couldn't create %s [%d]\n", path, res);
		}
	}

	return res;
}


static int test_rename_dir(void)
{
	const char *dn = FATFS_MNTP"/td";
	const char *ndn = FATFS_MNTP"/ntd";
	int res = TC_FAIL;

	TC_PRINT("\nRename directory tests:\n");

	if (delete_it(dn, 0) || delete_it(ndn, 0)) {
		goto cleanup;
	}

	/* Rename non-existing dir to non-existing dir */
	res = fs_rename(dn, ndn);
	if (!res) {
		TC_PRINT("Renamed non-existent directory\n");
		res = TC_FAIL;
		goto cleanup;
	}

	/* Rename existing dir to non-existing dir */
	res = create_dir(dn);
	if (!!res) {
		goto cleanup;
	}

	res = fs_rename(dn, ndn);
	if (!!res ||
	    !check_file_dir_exists(ndn) ||
	    check_file_dir_exists(dn)) {
		TC_PRINT("Renaming %s to %s failed [%d]\n", dn, ndn, res);
		res = TC_FAIL;
		goto cleanup;
	}

	/* Rename existing file to existing file */
	create_file(dn);
	res = fs_rename(dn, ndn);
	if (!!res ||
	    !check_file_dir_exists(ndn) ||
	    check_file_dir_exists(dn)) {
		TC_PRINT("Renaming %s to %s failed [%d]\n", dn, ndn, res);
		res = TC_FAIL;
		goto cleanup;
	}

cleanup:
	delete_it(dn, 1);
	delete_it(ndn, 1);

	return res;
}

static int test_rename_file(void)
{
	const char *fn = FATFS_MNTP"/tf.txt";
	const char *nfn = FATFS_MNTP"/ntf.txt";
	int res = TC_FAIL;

	TC_PRINT("\nRename file tests:\n");

	if (delete_it(fn, 0) || delete_it(nfn, 0)) {
		goto cleanup;
	}

	/* Rename non-existing file to non-existing file */
	res = fs_rename(fn, nfn);
	if (!res) {
		TC_PRINT("Renamed non-existent file\n");
		res = TC_FAIL;
		goto cleanup;
	}

	/* Rename existing file to non-existing file */
	res = create_file(fn);
	if (!!res) {
		goto cleanup;
	}

	res = fs_rename(fn, nfn);
	if (!!res ||
	    !check_file_dir_exists(nfn) ||
	    check_file_dir_exists(fn)) {
		TC_PRINT("Renaming %s to %s failed [%d]\n", fn, nfn, res);
		res = TC_FAIL;
		goto cleanup;
	}

	/* Rename existing file to existing file */
	create_file(fn);
	res = fs_rename(fn, nfn);
	if (!!res ||
	    !check_file_dir_exists(nfn) ||
	    check_file_dir_exists(fn)) {
		TC_PRINT("Renaming %s to %s failed [%d]\n", fn, nfn, res);
		res = TC_FAIL;
		goto cleanup;
	}

cleanup:
	delete_it(fn, 1);
	delete_it(nfn, 1);

	return res;
}

void test_fat_rename(void)
{
	zassert_true(test_rename_file() == TC_PASS, NULL);
	zassert_true(test_rename_dir() == TC_PASS, NULL);
}
