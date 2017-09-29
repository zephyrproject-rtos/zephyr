/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE &file
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

void test_append(void)
{
	fs_file_t file;
	struct nffs_file *nffs_file;
	struct fs_dirent info;
	char c;
	int rc;
	int i;

	static const struct nffs_area_desc area_descs_append[] = {
		{ 0x00000000 + TEST_FLASH_OFFSET, 128 * 1024 },
		{ 0x00020000 + TEST_FLASH_OFFSET, 128 * 1024 },
		{ 0, 0 },
	};

	rc = nffs_format_full(area_descs_append);
	zassert_equal(rc, 0, "cannot format to nffs");

	rc = fs_open(&file, "/myfile.txt");
	zassert_equal(rc, 0, "cannot open file");
	nffs_file = file.fp;
	nffs_test_util_assert_file_len(nffs_file, 0);
	zassert_equal(fs_tell(&file), 0, "invalid file length");

	rc = fs_write(&file, "abcdefgh", 8);
	nffs_test_util_assert_file_len(nffs_file, 8);
	zassert_equal(fs_tell(&file), 8, "invalid file size");
	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close file");

	nffs_test_util_assert_contents("/myfile.txt", "abcdefgh", 8);
	rc = fs_open(&file, "/myfile.txt");
	zassert_equal(rc, 0, "cannot open file");
	rc = fs_seek(&file, 0, FS_SEEK_END);
	zassert_equal(rc, 0, "cannot seek file");
	nffs_test_util_assert_file_len(nffs_file, 8);
	zassert_equal(fs_tell(&file), 8, "invalid file length");

	/*
	 * File position should always be at the end of a file after an append.
	 * Seek to the middle prior to writing to test this.
	 */
	rc = fs_seek(&file, 2, FS_SEEK_SET);
	zassert_equal(rc, 0, "cannot set position");
	nffs_test_util_assert_file_len(nffs_file, 8);
	zassert_equal(fs_tell(&file), 2, "invalid file length");

	rc = fs_seek(&file, 0, FS_SEEK_END);
	zassert_equal(rc, 0, "cannot seek file");
	rc = fs_write(&file, "ijklmnop", 8);
	nffs_test_util_assert_file_len(nffs_file, 16);
	zassert_equal(fs_tell(&file), 16, "invalid file length");
	rc = fs_write(&file, "qrstuvwx", 8);
	nffs_test_util_assert_file_len(nffs_file, 24);
	zassert_equal(fs_tell(&file), 24, "invalid file length");
	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close file");

	nffs_test_util_assert_contents("/myfile.txt",
						"abcdefghijklmnopqrstuvwx", 24);

	rc = fs_mkdir("/mydir");
	zassert_equal(rc, 0, "cannot create directory");
	rc = fs_open(&file, "/mydir/gaga.txt");
	zassert_equal(rc, 0, "cannot open file");

	/*** Repeated appends to a large file. */
	for (i = 0; i < 1000; i++) {
		fs_stat("/mydir/gaga.txt", &info);
		zassert_equal(info.size, i, "file lengths not matching");

		c = '0' + i % 10;
		rc = fs_write(&file, &c, 1);
	}

	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close file");

	nffs_test_util_assert_contents("/mydir/gaga.txt",
	"01234567890123456789012345678901234567890123456789" /* 1 */
	"01234567890123456789012345678901234567890123456789" /* 2 */
	"01234567890123456789012345678901234567890123456789" /* 3 */
	"01234567890123456789012345678901234567890123456789" /* 4 */
	"01234567890123456789012345678901234567890123456789" /* 5 */
	"01234567890123456789012345678901234567890123456789" /* 6 */
	"01234567890123456789012345678901234567890123456789" /* 7 */
	"01234567890123456789012345678901234567890123456789" /* 8 */
	"01234567890123456789012345678901234567890123456789" /* 9 */
	"01234567890123456789012345678901234567890123456789" /* 10 */
	"01234567890123456789012345678901234567890123456789" /* 11 */
	"01234567890123456789012345678901234567890123456789" /* 12 */
	"01234567890123456789012345678901234567890123456789" /* 13 */
	"01234567890123456789012345678901234567890123456789" /* 14 */
	"01234567890123456789012345678901234567890123456789" /* 15 */
	"01234567890123456789012345678901234567890123456789" /* 16 */
	"01234567890123456789012345678901234567890123456789" /* 17 */
	"01234567890123456789012345678901234567890123456789" /* 18 */
	"01234567890123456789012345678901234567890123456789" /* 19 */
	"01234567890123456789012345678901234567890123456789" /* 20 */
	,
	1000);

	struct nffs_test_file_desc *expected_system =
		(struct nffs_test_file_desc[]) { {
			.filename = "",
			.is_dir = 1,
			.children = (struct nffs_test_file_desc[]) { {
				.filename = "myfile.txt",
				.contents = "abcdefghijklmnopqrstuvwx",
				.contents_len = 24,
			}, {
				.filename = "mydir",
				.is_dir = 1,
				.children = (struct nffs_test_file_desc[]) { {
					.filename = "gaga.txt",
					.contents =
					"01234567890123456789012345678901234567890123456789"
					"01234567890123456789012345678901234567890123456789"
					"01234567890123456789012345678901234567890123456789"
					"01234567890123456789012345678901234567890123456789"
					"01234567890123456789012345678901234567890123456789"
					"01234567890123456789012345678901234567890123456789"
					"01234567890123456789012345678901234567890123456789"
					"01234567890123456789012345678901234567890123456789"
					"01234567890123456789012345678901234567890123456789"
					"01234567890123456789012345678901234567890123456789"
					"01234567890123456789012345678901234567890123456789"
					"01234567890123456789012345678901234567890123456789"
					"01234567890123456789012345678901234567890123456789"
					"01234567890123456789012345678901234567890123456789"
					"01234567890123456789012345678901234567890123456789"
					"01234567890123456789012345678901234567890123456789"
					"01234567890123456789012345678901234567890123456789"
					"01234567890123456789012345678901234567890123456789"
					"01234567890123456789012345678901234567890123456789"
					"01234567890123456789012345678901234567890123456789"
					,
					.contents_len = 1000,
				}, {
					.filename = NULL,
				} },
			}, {
				.filename = NULL,
			} },
		} };
	nffs_test_assert_system(expected_system, nffs_current_area_descs);
}
