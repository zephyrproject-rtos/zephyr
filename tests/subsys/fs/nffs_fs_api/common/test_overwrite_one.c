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

void test_overwrite_one(void)
{
	struct fs_file_t file;
	struct nffs_file *nffs_file;
	int rc;

	/*** Setup. */
	rc = nffs_format_full(nffs_current_area_descs);
	zassert_equal(rc, 0, "cannot format nffs");

	nffs_test_util_append_file(NFFS_MNTP"/myfile.txt", "abcdefgh", 8);

	/*** Overwrite within one block (middle). */
	rc = fs_open(&file, NFFS_MNTP"/myfile.txt");
	nffs_file = file.filep;
	zassert_equal(rc, 0, "cannot open file");
	nffs_test_util_assert_file_len(nffs_file, 8);
	zassert_equal(fs_tell(&file), 0, "invalid pos in file");

	rc = fs_seek(&file, 3, FS_SEEK_SET);
	zassert_equal(rc, 0, "cannot set pos in file");
	nffs_test_util_assert_file_len(nffs_file, 8);
	zassert_equal(fs_tell(&file), 3, "invalid pos in file");

	rc = fs_write(&file, "12", 2);
	nffs_test_util_assert_file_len(nffs_file, 8);
	zassert_equal(fs_tell(&file), 5, "cannot get pos in file");
	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close file");

	nffs_test_util_assert_contents(NFFS_MNTP"/myfile.txt", "abc12fgh", 8);
	nffs_test_util_assert_block_count(NFFS_MNTP"/myfile.txt", 1);

	/*** Overwrite within one block (start). */
	rc = fs_open(&file, NFFS_MNTP"/myfile.txt");
	zassert_equal(rc, 0, "cannot open file");
	nffs_test_util_assert_file_len(nffs_file, 8);
	zassert_equal(fs_tell(&file), 0, "invalid pos in file");

	rc = fs_write(&file, "xy", 2);
	nffs_test_util_assert_file_len(nffs_file, 8);
	zassert_equal(fs_tell(&file), 2, "invalid pos in file");
	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close file");

	nffs_test_util_assert_contents(NFFS_MNTP"/myfile.txt", "xyc12fgh", 8);
	nffs_test_util_assert_block_count(NFFS_MNTP"/myfile.txt", 1);

	/*** Overwrite within one block (end). */
	rc = fs_open(&file, NFFS_MNTP"/myfile.txt");
	zassert_equal(rc, 0, "cannot open file");
	nffs_test_util_assert_file_len(nffs_file, 8);
	zassert_equal(fs_tell(&file), 0, "invalid pos in file");

	rc = fs_seek(&file, 6, FS_SEEK_SET);
	zassert_equal(rc, 0, "cannot set pos in file");
	nffs_test_util_assert_file_len(nffs_file, 8);
	zassert_equal(fs_tell(&file), 6, "invalid pos in file");

	rc = fs_write(&file, "<>", 2);
	nffs_test_util_assert_file_len(nffs_file, 8);
	zassert_equal(fs_tell(&file), 8, "invalid pos in file");
	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close file");

	nffs_test_util_assert_contents(NFFS_MNTP"/myfile.txt", "xyc12f<>", 8);
	nffs_test_util_assert_block_count(NFFS_MNTP"/myfile.txt", 1);

	/*** Overwrite one block middle, extend. */
	rc = fs_open(&file, NFFS_MNTP"/myfile.txt");
	zassert_equal(rc, 0, "cannot open file");
	nffs_test_util_assert_file_len(nffs_file, 8);
	zassert_equal(fs_tell(&file), 0, "invalid pos in file");

	rc = fs_seek(&file, 4, FS_SEEK_SET);
	zassert_equal(rc, 0, "cannot set pos in file");
	nffs_test_util_assert_file_len(nffs_file, 8);
	zassert_equal(fs_tell(&file), 4, "invalid pos in file");

	rc = fs_write(&file, "abcdefgh", 8);
	nffs_test_util_assert_file_len(nffs_file, 12);
	zassert_equal(fs_tell(&file), 12, "invalid pos in file");
	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close file");

	nffs_test_util_assert_contents(NFFS_MNTP"/myfile.txt",
						"xyc1abcdefgh", 12);
	nffs_test_util_assert_block_count(NFFS_MNTP"/myfile.txt", 1);

	/*** Overwrite one block start, extend. */
	rc = fs_open(&file, NFFS_MNTP"/myfile.txt");
	zassert_equal(rc, 0, "cannot open file");
	nffs_test_util_assert_file_len(nffs_file, 12);
	zassert_equal(fs_tell(&file), 0, "invalid pos in file");

	rc = fs_write(&file, "abcdefghijklmnop", 16);
	nffs_test_util_assert_file_len(nffs_file, 16);
	zassert_equal(fs_tell(&file), 16, "invalid pos in file");
	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close file");

	nffs_test_util_assert_contents(NFFS_MNTP"/myfile.txt",
						"abcdefghijklmnop", 16);
	nffs_test_util_assert_block_count(NFFS_MNTP"/myfile.txt", 1);

	struct nffs_test_file_desc *expected_system =
		(struct nffs_test_file_desc[]) { {
			.filename = "",
			.is_dir = 1,
			.children = (struct nffs_test_file_desc[]) { {
				.filename = "myfile.txt",
				.contents = "abcdefghijklmnop",
				.contents_len = 16,
			}, {
				.filename = NULL,
			} },
		} };

	nffs_test_assert_system(expected_system, nffs_current_area_descs);
}
