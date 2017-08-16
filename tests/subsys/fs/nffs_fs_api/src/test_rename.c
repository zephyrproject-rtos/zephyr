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
#include <fs.h>
#include "nffs_test_utils.h"
#include <ztest_assert.h>

void test_rename(void)
{
	struct fs_dirent file;
	const char contents[] = "contents";
	int rc;

	rc = nffs_format_full(nffs_current_area_descs);
	zassert_equal(rc, 0, "cannot format nffs");

	rc = nffs_path_rename("/nonexistent.txt", "/newname.txt");
	zassert_equal(rc, FS_ENOENT, "cannot rename file");

	/*** Rename file. */
	nffs_test_util_create_file("/myfile.txt", contents, sizeof(contents));

	rc = nffs_path_rename("/myfile.txt", "badname");
	zassert_equal(rc, FS_EINVAL, "cannot rename file");

	rc = nffs_path_rename("/myfile.txt", "/myfile2.txt");
	zassert_equal(rc, 0, "cannot rename file");

	rc = fs_stat("/myfile.txt", &file);
	zassert_not_equal(rc, 0, "cannot open file");

	nffs_test_util_assert_contents("/myfile2.txt", contents,
							sizeof(contents));

	rc = fs_mkdir("/mydir");
	zassert_equal(rc, 0, "cannot create directory");

	rc = fs_mkdir("/mydir/leafdir");
	zassert_equal(rc, 0, "cannot create sub-directory");

	rc = nffs_path_rename("/myfile2.txt", "/mydir/myfile2.txt");
	zassert_equal(rc, 0, "cannot rename file in sub-directory");

	nffs_test_util_assert_contents("/mydir/myfile2.txt", contents,
							sizeof(contents));

	/*** Rename directory. */
	rc = nffs_path_rename("/mydir", "badname");
	zassert_equal(rc, FS_EINVAL, "cannot rename directory");

	/* Don't allow a directory to be moved into a descendent directory. */
	rc = nffs_path_rename("/mydir", "/mydir/leafdir/a");
	zassert_equal(rc, FS_EINVAL, "directory moved to decendent dir");

	rc = nffs_path_rename("/mydir", "/mydir2");
	zassert_equal(rc, 0, "cannot rename directory");

	nffs_test_util_assert_contents("/mydir2/myfile2.txt", contents,
							sizeof(contents));

	struct nffs_test_file_desc *expected_system =
		(struct nffs_test_file_desc[]) { {
			.filename = "",
			.is_dir = 1,
			.children = (struct nffs_test_file_desc[]) { {
				.filename = "mydir2",
				.is_dir = 1,
				.children = (struct nffs_test_file_desc[]) { {
					.filename = "leafdir",
					.is_dir = 1,
				}, {
					.filename = "myfile2.txt",
					.contents = "contents",
					.contents_len = 9,
				}, {
					.filename = NULL,
				} },
			}, {
				.filename = NULL,
			} },
		} };

	nffs_test_assert_system(expected_system, nffs_current_area_descs);
}
