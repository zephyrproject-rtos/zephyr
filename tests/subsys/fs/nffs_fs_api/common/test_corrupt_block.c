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
#include <nffs/os.h>
#include <fs/fs.h>
#include "nffs_test_utils.h"
#include <ztest.h>
#include <ztest_assert.h>

static u8_t data1[] = {0x43};

void test_corrupt_block(void)
{
	struct fs_file_t fs_file;
	struct nffs_file *file;
	struct nffs_block block;
	uint32_t flash_offset;
	uint32_t area_offset;
	uint8_t area_idx;
	u8_t off;    /* offset to corrupt */
	int rc;
	struct nffs_disk_block ndb;

	/* Setup. */
	rc = nffs_format_full(nffs_current_area_descs);
	zassert_equal(rc, 0, NULL);

	rc = fs_mkdir(NFFS_MNTP"/mydir");
	zassert_equal(rc, 0, "cannot create directory");

	nffs_test_util_create_file(NFFS_MNTP"/mydir/a", "aaaa", 4);
	nffs_test_util_create_file(NFFS_MNTP"/mydir/b", "bbbb", 4);
	nffs_test_util_create_file(NFFS_MNTP"/mydir/c", "cccc", 4);

	/* Add a second block to the 'b' file. */
	nffs_test_util_append_file(NFFS_MNTP"/mydir/b", "1234", 4);

	/* Corrupt the 'b' file; overwrite the second block's magic number. */
	rc = fs_open(&fs_file, NFFS_MNTP"/mydir/b");
	zassert_equal(rc, 0, "cannot open file");

	file = fs_file.filep;
	rc = nffs_block_from_hash_entry(&block,
				file->nf_inode_entry->nie_last_block_entry);
	zassert_equal(rc, 0, "block from hash entry error");

	nffs_flash_loc_expand(block.nb_hash_entry->nhe_flash_loc, &area_idx,
								&area_offset);
	flash_offset = nffs_areas[area_idx].na_offset + area_offset;

	/*
	 * Overwriting the reserved16 field should invalidate the CRC
	 */
	off = (char *)&ndb.reserved16 - (char *)&ndb;
	nffs_test_util_overwrite_data(data1, sizeof(data1), flash_offset + off);

	zassert_equal(rc, 0, "flash native memset error");

	/*
	 * Write a fourth file. This file should get restored even though the
	 * previous object has an invalid magic number.
	 */
	nffs_test_util_create_file(NFFS_MNTP"/mydir/d", "dddd", 4);

	rc = nffs_misc_reset();
	zassert_equal(rc, 0, "cannot reset nffs");
	rc = nffs_restore_full(nffs_current_area_descs);
	zassert_equal(rc, 0, "cannot detect nffs");

	/*
	 * The entire second block should be removed; the file should only
	 * contain the first block.
	 */
	struct nffs_test_file_desc *expected_system =
	(struct nffs_test_file_desc[]) { {
		.filename = "",
		.is_dir = 1,
		.children = (struct nffs_test_file_desc[]) { {
			.filename = "mydir",
			.is_dir = 1,
			.children = (struct nffs_test_file_desc[]) { {
				.filename = "a",
				.contents = "aaaa",
				.contents_len = 4,
			}, {
				.filename = "c",
				.contents = "cccc",
				.contents_len = 4,
			}, {
				.filename = "d",
				.contents = "dddd",
				.contents_len = 4,
			}, {
				.filename = NULL,
			} },
		}, {
			.filename = NULL,
		} },
	} };

	nffs_test_assert_system(expected_system, nffs_current_area_descs);
}
