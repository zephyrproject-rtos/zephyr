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
#include <stdio.h>
#include <nffs/nffs.h>
#include <fs.h>
#include "nffs_test_utils.h"
#include <ztest.h>
#include <ztest_assert.h>


void test_open(void)
{
	struct fs_file_t file;
	struct fs_dir_t dir;
	int rc;

	rc = nffs_format_full(nffs_current_area_descs);
	zassert_equal(rc, 0, "cannot format nffs");

	/*** Fail to open an invalid path (not rooted). */
	rc = fs_open(&file, "file");
	zassert_equal(rc, -EINVAL, "failed to detect invalid path");

	/*** Fail to open a directory (root directory). */
	rc = fs_open(&file, "/");
	zassert_equal(rc, -EINVAL, "failed to detect invalid directory");

	/*** Fail to open a child of a nonexistent directory. */
	rc = fs_open(&file, "/dir/myfile.txt");
	zassert_equal(rc, -ENOENT, "failed to detect nonexistent directory");
	rc = fs_opendir(&dir, "/dir");
	zassert_equal(rc, -ENOENT, "failed to detect nonexistent directory");

	rc = fs_mkdir(NFFS_MNTP"/dir");
	zassert_equal(rc, 0, "failed to open directory");

	/*** Fail to open a directory. */
	rc = fs_open(&file, NFFS_MNTP"/dir");
	zassert_equal(rc, -EINVAL, "failed to open a directory");

	/*** Successfully open an existing file for reading. */
	nffs_test_util_create_file(NFFS_MNTP"/dir/file.txt", "1234567890", 10);
	rc = fs_open(&file, NFFS_MNTP"/dir/file.txt");
	zassert_equal(rc, 0, "failed to open a file");
	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close file");

	/*** Successfully open an nonexistent file for writing. */
	rc = fs_open(&file, NFFS_MNTP"/dir/file2.txt");
	zassert_equal(rc, 0, "cannot open nonexistent file for writing");
	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close file");

	/*** Ensure the file can be reopened. */
	rc = fs_open(&file, NFFS_MNTP"/dir/file.txt");
	zassert_equal(rc, 0, "cannot reopen file");
	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close reopened file");
}
