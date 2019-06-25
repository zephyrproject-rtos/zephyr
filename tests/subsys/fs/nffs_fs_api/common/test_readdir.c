/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <nffs/nffs.h>
#include <fs/fs.h>
#include "nffs_test_utils.h"
#include <ztest_assert.h>


void test_readdir(void)
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
