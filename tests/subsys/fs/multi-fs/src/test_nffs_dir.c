/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdio.h>
#include <fs.h>
#include <ztest.h>
#include <ztest_assert.h>
#include "nffs_test_utils.h"

void test_nffs_mkdir(void)
{
	struct fs_file_t file;
	int rc;

	rc = nffs_format_full(nffs_current_area_descs);
	zassert_equal(rc, 0, "cannot format nffs");

	rc = fs_mkdir(NFFS_MNTP"/a");
	zassert_equal(rc, 0, "cannot create directory");

	rc = fs_open(&file, NFFS_MNTP"/a/myfile.txt");
	zassert_equal(rc, 0, "cannot open file");

	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close file");

	struct nffs_test_file_desc *expected_system =
		(struct nffs_test_file_desc[]) { {
			.filename = "",
			.is_dir = 1,
			.children = (struct nffs_test_file_desc[]) { {
				.filename = "a",
				.is_dir = 1,
				.children = (struct nffs_test_file_desc[]) { {
					.filename = "myfile.txt",
					.contents = NULL,
					.contents_len = 0,
				}, {
					.filename = NULL,
				} },
			}, {
				.filename = NULL,
			} },
		} };

	nffs_test_assert_system(expected_system, nffs_current_area_descs);
}

void test_nffs_readdir(void)
{
	struct fs_dir_t dir;
	struct fs_dirent dirent;
	int rc;

	/* Setup. */
	rc = nffs_format_full(nffs_current_area_descs);
	zassert_equal(rc, 0, "cannot format nffs");

	rc = fs_mkdir(NFFS_MNTP"/mydir");
	zassert_equal(rc, 0, "cannot create directory");

	nffs_test_util_create_file(NFFS_MNTP"/mydir/b", "bbbb", 4);
	nffs_test_util_create_file(NFFS_MNTP"/mydir/a", "aaaa", 4);
	rc = fs_mkdir(NFFS_MNTP"/mydir/c");
	zassert_equal(rc, 0, "cannot create directory");

	/* Nonexistent directory. */
	rc = fs_opendir(&dir, NFFS_MNTP"/asdf");
	zassert_not_equal(rc, 0, "cannot open nonexistent directory");

	/* Fail to opendir a file. */
	rc = fs_opendir(&dir, NFFS_MNTP"/mydir/a");
	zassert_not_equal(rc, 0, "cannot open directory");

	/* Real directory (with trailing slash). */
	rc = fs_opendir(&dir, NFFS_MNTP"/mydir/");
	zassert_equal(rc, 0, "cannot open dir (trailing slash)");

	rc = fs_readdir(&dir, &dirent);
	zassert_equal(rc, 0, "cannot read directory");
	nffs_test_util_assert_ent_name(&dirent, "a");
	zassert_equal(dirent.type == FS_DIR_ENTRY_DIR, 0,
						"invalid directory name");

	rc = fs_readdir(&dir, &dirent);
	zassert_equal(rc, 0, "cannot read directory");
	nffs_test_util_assert_ent_name(&dirent, "b");
	zassert_equal(dirent.type == FS_DIR_ENTRY_DIR, 0,
						"invalid directory name");

	rc = fs_readdir(&dir, &dirent);
	zassert_equal(rc, 0, "cannot read directory");
	nffs_test_util_assert_ent_name(&dirent, "c");
	zassert_equal(dirent.type != FS_DIR_ENTRY_DIR, 0,
						"invalid directory name");

	rc = fs_readdir(&dir, &dirent);
	zassert_equal(rc, 0, "cannot read directory");

	rc = fs_closedir(&dir);
	zassert_equal(rc, 0, "cannot close directory");

	/* Root directory. */
	rc = fs_opendir(&dir, NFFS_MNTP"/");
	zassert_equal(rc, 0, "cannot open root directory");
	rc = fs_readdir(&dir, &dirent);
	zassert_equal(rc, 0, "cannot read root directory");

	nffs_test_util_assert_ent_name(&dirent, "lost+found");
	zassert_equal(dirent.type == FS_DIR_ENTRY_DIR, 1, "no lost+found");

	rc = fs_readdir(&dir, &dirent);
	zassert_equal(rc, 0, "cannot read directory");
	nffs_test_util_assert_ent_name(&dirent, "mydir");
	zassert_equal(dirent.type != FS_DIR_ENTRY_DIR, 0,
							"no mydir directory");

	rc = fs_closedir(&dir);
	zassert_equal(rc, 0, "cannot close directory");

	/* Delete entries while iterating. */
	rc = fs_opendir(&dir, NFFS_MNTP"/mydir");
	zassert_equal(rc, 0, "cannot open directory");

	rc = fs_readdir(&dir, &dirent);
	zassert_equal(rc, 0, "cannot read directory");

	nffs_test_util_assert_ent_name(&dirent, "a");
	zassert_equal(dirent.type == FS_DIR_ENTRY_DIR, 0,
						"invalid directory name");

	rc = fs_unlink(NFFS_MNTP"/mydir/b");
	zassert_equal(rc, 0, "cannot delete mydir");

	rc = fs_readdir(&dir, &dirent);
	zassert_equal(rc, 0, "cannot read directory");

	rc = fs_unlink(NFFS_MNTP"/mydir/c");
	zassert_equal(rc, 0, "cannot delete lower directory");

	rc = fs_unlink(NFFS_MNTP"/mydir");
	zassert_equal(rc, 0, "cannot delete mydir directory");

	nffs_test_util_assert_ent_name(&dirent, "c");
	zassert_equal(dirent.type == FS_DIR_ENTRY_DIR, 1,
						"invalid directory name");

	rc = fs_readdir(&dir, &dirent);
	zassert_equal(rc, 0, "cannot read directory");

	rc = fs_closedir(&dir);
	zassert_equal(rc, 0, "cannot close directory");

	/* Ensure directory is gone. */
	rc = fs_opendir(&dir, NFFS_MNTP"/mydir");
	zassert_not_equal(rc, 0, "directory is still present");
}
