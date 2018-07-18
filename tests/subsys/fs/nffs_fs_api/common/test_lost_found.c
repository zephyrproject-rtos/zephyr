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
#include <nffs/os.h>
#include <fs.h>
#include "nffs_test_utils.h"
#include <ztest.h>
#include <ztest_assert.h>

static u8_t data1[] = {0xaa};

void test_lost_found(void)
{
	char buf[32];
	struct nffs_inode_entry *inode_entry;
	uint32_t flash_offset;
	uint32_t area_offset;
	uint8_t area_idx;
	int rc;
	struct nffs_disk_inode ndi;
	u8_t off;    /* calculated offset for memset */

	/*** Setup. */
	rc = nffs_format_full(nffs_current_area_descs);
	zassert_equal(rc, 0, "cannot format nffs");

	rc = fs_mkdir(NFFS_MNTP"/mydir");
	zassert_equal(rc, 0, "cannot create directory");
	rc = fs_mkdir(NFFS_MNTP"/mydir/dir1");
	zassert_equal(rc, 0, "cannot create directory");

	nffs_test_util_create_file(NFFS_MNTP"/mydir/file1", "aaaa", 4);
	nffs_test_util_create_file(NFFS_MNTP"/mydir/dir1/file2", "bbbb", 4);

	/* Corrupt the mydir inode. */
	rc = nffs_path_find_inode_entry("/mydir", &inode_entry);
	zassert_equal(rc, 0, "path to find inode error");

	snprintf(buf, sizeof(buf), "%lu",
	(unsigned long)inode_entry->nie_hash_entry.nhe_id);

	nffs_flash_loc_expand(inode_entry->nie_hash_entry.nhe_flash_loc,
						&area_idx, &area_offset);
	flash_offset = nffs_areas[area_idx].na_offset + area_offset;

	/*
	 * Overwrite the sequence number - should be detected as CRC corruption
	 */
	off = (char *)&ndi.ndi_seq - (char *)&ndi;

	nffs_test_util_overwrite_data(data1, sizeof(data1), flash_offset + off);

	/* Clear cached data and restore from flash (i.e, simulate a reboot). */
	rc = nffs_misc_reset();
	zassert_equal(rc, 0, "nffs reset error");
	rc = nffs_restore_full(nffs_current_area_descs);
	zassert_equal(rc, 0, "nffs detect error");

	/* All contents should now be in the lost+found dir. */
	struct nffs_test_file_desc *expected_system =
		(struct nffs_test_file_desc[]) { {
			.filename = "",
			.is_dir = 1,
			.children = (struct nffs_test_file_desc[]) { {
				.filename = "lost+found",
				.is_dir = 1,
			}, {
				.filename = NULL,
			} }
		} };

	nffs_test_assert_system(expected_system, nffs_current_area_descs);
}
