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

#define TEST_CONTENT_LEN (NFFS_BLOCK_MAX_DATA_SZ_MAX * 5)

void test_large_write(void)
{
	int rc;
	int i;

	static const struct nffs_area_desc area_descs_two[] = {
		{ 0x00000000 + TEST_FLASH_OFFSET, 128 * 1024 },
		{ 0x00020000 + TEST_FLASH_OFFSET, 128 * 1024 },
		{ 0, 0 },
	};

	/*** Setup. */
	rc = nffs_format_full(area_descs_two);
	zassert_equal(rc, 0, "cannot format nffs");

	for (i = 0; i < TEST_CONTENT_LEN; i++) {
		nffs_test_buf[i] = i;
	}

	nffs_test_util_create_file(NFFS_MNTP"/myfile.txt", nffs_test_buf,
							TEST_CONTENT_LEN);

	/*
	 * Ensure large write was split across the appropriate number of data
	 * blocks.
	 */
	zassert_equal(nffs_test_util_block_count(NFFS_MNTP"/myfile.txt"),
				TEST_CONTENT_LEN / NFFS_BLOCK_MAX_DATA_SZ_MAX,
				"blocks were not split");

	/*
	 * Garbage collect and then ensure the large file is still properly
	 * divided according to max data block size.
	 */
	nffs_gc(NULL);
	zassert_equal(nffs_test_util_block_count(NFFS_MNTP"/myfile.txt"),
				TEST_CONTENT_LEN / NFFS_BLOCK_MAX_DATA_SZ_MAX,
				"not properly divided");

	struct nffs_test_file_desc *expected_system =
		(struct nffs_test_file_desc[]) { {
			.filename = "",
			.is_dir = 1,
			.children = (struct nffs_test_file_desc[]) { {
				.filename = "myfile.txt",
				.contents = nffs_test_buf,
				.contents_len = TEST_CONTENT_LEN,
			}, {
				.filename = NULL,
			} },
		} };

	nffs_test_assert_system(expected_system, area_descs_two);
}
