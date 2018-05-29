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

void test_overwrite_many(void)
{
	struct nffs_test_block_desc *blocks = (struct nffs_test_block_desc[]) { {
		.data = "abcdefgh",
		.data_len = 8,
	}, {
		.data = "ijklmnop",
		.data_len = 8,
	}, {
		.data = "qrstuvwx",
		.data_len = 8,
	} };

	struct fs_file_t file;
	struct nffs_file *nffs_file;
	int rc;

	/*** Setup. */
	rc = nffs_format_full(nffs_current_area_descs);
	zassert_equal(rc, 0, "cannot format nffs");

	/*** Overwrite middle of first block. */
	nffs_test_util_create_file_blocks(NFFS_MNTP"/myfile.txt", blocks, 3);

	rc = fs_open(&file, NFFS_MNTP"/myfile.txt");
	nffs_file = file.filep;
	zassert_equal(rc, 0, "cannot open file");
	nffs_test_util_assert_file_len(nffs_file, 24);
	zassert_equal(fs_tell(&file), 0, "invalid file length");
	rc = fs_seek(&file, 3, FS_SEEK_SET);

	zassert_equal(rc, 0, "cannot set pos in file");
	nffs_test_util_assert_file_len(nffs_file, 24);
	zassert_equal(fs_tell(&file), 3, "invalid pos in file");

	rc = fs_write(&file, "12", 2);
	nffs_test_util_assert_file_len(nffs_file, 24);
	zassert_equal(fs_tell(&file), 5, "invalid file length");

	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close file");
	nffs_test_util_assert_contents(NFFS_MNTP"/myfile.txt",
					"abc12fghijklmnopqrstuvwx", 24);
	nffs_test_util_assert_block_count(NFFS_MNTP"/myfile.txt", 3);

	/*** Overwrite end of first block, start of second. */
	nffs_test_util_create_file_blocks(NFFS_MNTP"/myfile.txt", blocks, 3);

	rc = fs_open(&file, NFFS_MNTP"/myfile.txt");
	zassert_equal(rc, 0, "cannot open file");
	nffs_test_util_assert_file_len(nffs_file, 24);
	zassert_equal(fs_tell(&file), 0, "invalid file length");

	rc = fs_seek(&file, 6, FS_SEEK_SET);
	zassert_equal(rc, 0, "cannot set pos in file");
	nffs_test_util_assert_file_len(nffs_file, 24);
	zassert_equal(fs_tell(&file), 6, "invalid pos in file");

	rc = fs_write(&file, "1234", 4);
	nffs_test_util_assert_file_len(nffs_file, 24);
	zassert_equal(fs_tell(&file), 10, "invalid pos in file");

	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close file");
	nffs_test_util_assert_contents(NFFS_MNTP"/myfile.txt",
					"abcdef1234klmnopqrstuvwx", 24);
	nffs_test_util_assert_block_count(NFFS_MNTP"/myfile.txt", 3);
	struct nffs_test_file_desc *expected_system =
		(struct nffs_test_file_desc[]) { {
			.filename = "",
			.is_dir = 1,
			.children = (struct nffs_test_file_desc[]) { {
				.filename = "myfile.txt",
				.contents = "abcdef1234klmnopqrstuvwx",
				.contents_len = 24,
			}, {
				.filename = NULL,
			} },
		} };
	nffs_test_assert_system(expected_system, nffs_current_area_descs);
}
