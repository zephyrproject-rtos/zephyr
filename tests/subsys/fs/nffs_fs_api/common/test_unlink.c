/*
 *Licensed to the Apache Software Foundation (ASF) under one
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

extern struct k_mem_slab nffs_block_entry_pool;
extern struct k_mem_slab nffs_inode_entry_pool;

void test_unlink(void)
{
	struct fs_dirent file_stats;
	struct fs_file_t file0, file1;
	u8_t buf[64];
	struct nffs_file *nffs_file;
	u32_t bytes_read;
	int initial_num_blocks;
	int initial_num_inodes;
	int rc;

	rc = nffs_format_full(nffs_current_area_descs);
	zassert_equal(rc, 0, "cannot format nffs");

	initial_num_blocks = k_mem_slab_num_free_get(&nffs_block_entry_pool);
	initial_num_inodes = k_mem_slab_num_free_get(&nffs_inode_entry_pool);

	nffs_test_util_create_file(NFFS_MNTP"/file0.txt", "0", 1);

	rc = fs_open(&file0, NFFS_MNTP"/file0.txt");
	zassert_equal(rc, 0, "cannot open file");
	nffs_file = file0.filep;
	zassert_equal(nffs_file->nf_inode_entry->nie_refcnt, 2, "inode error");

	rc = fs_unlink(NFFS_MNTP"/file0.txt");
	zassert_equal(rc, 0, "");
	zassert_equal(nffs_file->nf_inode_entry->nie_refcnt, 1, "inode error");

	rc = fs_stat(NFFS_MNTP"/file0.txt", &file_stats);
	zassert_not_equal(rc, 0, "no such file");

	rc = fs_write(&file0, "00", 2);

	rc = fs_seek(&file0, 0, FS_SEEK_SET);
	zassert_equal(rc, 0, "cannot set pos in file");

	bytes_read = fs_read(&file0, buf, sizeof(buf));
	zassert_equal(bytes_read, 2, "invalid bytes read");
	zassert_equal(memcmp(buf, "00", 2), 0, "invalid buffer size");

	rc = fs_close(&file0);
	zassert_equal(rc, 0, "cannot close file");


	rc = fs_stat(NFFS_MNTP"/file0.txt", &file_stats);
	zassert_not_equal(rc, 0, "no such file");

	/* Ensure the file was fully removed from RAM. */
	zassert_equal(k_mem_slab_num_free_get(&nffs_inode_entry_pool),
				initial_num_inodes, "file not remove entirely");
	zassert_equal(k_mem_slab_num_free_get(&nffs_block_entry_pool),
				initial_num_blocks, "file not remove entirely");

	/*** Nested unlink. */
	rc = fs_mkdir(NFFS_MNTP"/mydir");
	zassert_equal(rc, 0, "cannot make directory");
	nffs_test_util_create_file(NFFS_MNTP"/mydir/file1.txt", "1", 2);

	rc = fs_open(&file1, NFFS_MNTP"/mydir/file1.txt");
	zassert_equal(rc, 0, "cannot open file");
	nffs_file = file1.filep;
	zassert_equal(nffs_file->nf_inode_entry->nie_refcnt, 2, "inode error");

	rc = fs_unlink(NFFS_MNTP"/mydir");
	zassert_equal(rc, 0, "cannot delete directory");
	zassert_equal(nffs_file->nf_inode_entry->nie_refcnt, 1, "inode error");

	rc = fs_stat(NFFS_MNTP"/mydir/file1.txt", &file_stats);
	zassert_not_equal(rc, 0, "unlink failed");

	rc = fs_write(&file1, "11", 2);

	rc = fs_seek(&file1, 0, FS_SEEK_SET);
	zassert_equal(rc, 0, "cannot set pos in file");

	bytes_read = fs_read(&file1, buf, sizeof(buf));
	zassert_equal(bytes_read, 2, "invalid bytes read");
	zassert_equal(memcmp(buf, "11", 2), 0, "invalid buffer size");

	rc = fs_close(&file1);
	zassert_equal(rc, 0, "cannot close file");

	rc = fs_stat(NFFS_MNTP"/mydir/file1.txt", &file_stats);
	zassert_not_equal(rc, 0, "unlink failed");

	struct nffs_test_file_desc *expected_system =
		(struct nffs_test_file_desc[]) { {
			.filename = "",
			.is_dir = 1,
		} };

	nffs_test_assert_system(expected_system, nffs_current_area_descs);

	/* Ensure the files and directories were fully removed from RAM. */
	zassert_equal(k_mem_slab_num_free_get(&nffs_inode_entry_pool),
				initial_num_inodes, "not all removed from RAM");
	zassert_equal(k_mem_slab_num_free_get(&nffs_block_entry_pool),
				initial_num_blocks, "not all removed from RAM");
}
