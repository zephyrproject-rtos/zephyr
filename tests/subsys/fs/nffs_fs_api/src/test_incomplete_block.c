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
#include <fs.h>
#include "nffs_test_utils.h"
#include <ztest.h>
#include <ztest_assert.h>

	/*
	 * This test no longer works with the current implementation. The
	 * expectation is that intermediate blocks can be removed and the old
	 * method of finding the last current block after restore will allow the
	 * file to be salvaged. Instead, the file should be removed and all data
	 * declared invalid.
	 */
void test_incomplete_block(void)
{
	static u8_t corrupt_data[] = { 0xff, 0xff };
	struct nffs_file *file;
	struct nffs_block block;
	fs_file_t fs_file;
	u32_t flash_offset;
	u32_t area_offset;
	u8_t area_idx;
	int rc;

	/*** Setup. */
	rc = nffs_format_full(nffs_current_area_descs);
	zassert_equal(rc, 0, "cannot format nffs");

	rc = fs_mkdir("/mydir");
	zassert_equal(rc, 0, "cannot create directory");

	nffs_test_util_create_file("/mydir/a", "aaaa", 4);
	nffs_test_util_create_file("/mydir/b", "bbbb", 4);
	nffs_test_util_create_file("/mydir/c", "cccc", 4);

	/* Add a second block to the 'b' file. */
	nffs_test_util_append_file("/mydir/b", "1234", 4);

	/*
	 * Corrupt the 'b' file; make it look like the second block only got
	 * half written.
	 */
	rc = fs_open(&fs_file, "/mydir/b");
	zassert_equal(rc, 0, "cannot open file");
	file = fs_file.fp;

	rc = nffs_block_from_hash_entry(&block,
				file->nf_inode_entry->nie_last_block_entry);
	zassert_equal(rc, 0, "block from hash entry error");

	nffs_flash_loc_expand(block.nb_hash_entry->nhe_flash_loc, &area_idx,
								&area_offset);
	flash_offset = nffs_areas[area_idx].na_offset + area_offset;

	/*
	 * Overwrite block data - the CRC check should pick this up
	 */
	nffs_test_util_overwrite_data(corrupt_data, sizeof(corrupt_data),
				      flash_offset +
				      sizeof(struct nffs_disk_block) + 2);

	rc = nffs_misc_reset();
	zassert_equal(rc, 0, "nffs misc reset error");
	rc = nffs_restore_full(nffs_current_area_descs);
	zassert_equal(rc, 0, "nffs detect error");

	/*
	 * OLD: The entire second block should be removed; the file should only
	 * contain the first block.
	 * Unless we can salvage the block, the entire file should probably be
	 * removed. This is a contrived example which generates bad data on the
	 * what happens to be the last block, but corruption can actually occur
	 * in any block. Sweep should be updated to search look for blocks that
	 * don't have a correct prev_id and then decide whether to delete the
	 * owning inode. XXX
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
					.filename = NULL,
				} },
			}, {
				.filename = NULL,
			} },
		} };

	nffs_test_assert_system(expected_system, nffs_current_area_descs);
}
