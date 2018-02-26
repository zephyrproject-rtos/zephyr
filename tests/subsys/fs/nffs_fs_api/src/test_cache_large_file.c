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

void test_cache_large_file(void)
{
	struct fs_file_t file;
	u8_t b;
	int rc;

	/*** Setup. */
	rc = nffs_format_full(nffs_current_area_descs);
	zassert_equal(rc, 0, "canot format nffs");

	nffs_test_util_create_file(NFFS_MNTP"/myfile.txt", nffs_test_buf,
						NFFS_BLOCK_MAX_DATA_SZ_MAX * 5);
	nffs_cache_clear();

	/* Opening a file should not cause any blocks to get cached. */
	rc = fs_open(&file, NFFS_MNTP"/myfile.txt");
	zassert_equal(rc, 0, "cannot open file");
	nffs_test_util_assert_cache_range(NFFS_MNTP"/myfile.txt", 0, 0);

	/* Cache first block. */
	rc = fs_seek(&file, nffs_block_max_data_sz * 0, FS_SEEK_SET);
	zassert_equal(rc, 0, "cannot set pos in file");
	rc = fs_read(&file, &b, 1);
	zassert_equal(rc, 1, "cannot read file");
	nffs_test_util_assert_cache_range(NFFS_MNTP"/myfile.txt",
						nffs_block_max_data_sz * 0,
						nffs_block_max_data_sz * 1);

	/* Cache second block. */
	rc = fs_seek(&file, nffs_block_max_data_sz * 1, FS_SEEK_SET);
	zassert_equal(rc, 0, "cannot set pos in file");
	rc = fs_read(&file, &b, 1);
	zassert_equal(rc, 1, "cannot read file");
	nffs_test_util_assert_cache_range(NFFS_MNTP"/myfile.txt",
						nffs_block_max_data_sz * 0,
						nffs_block_max_data_sz * 2);


	/* Cache fourth block; prior cache should get erased. */
	rc = fs_seek(&file, nffs_block_max_data_sz * 3, FS_SEEK_SET);
	zassert_equal(rc, 0, "cannot set pos in file");
	rc = fs_read(&file, &b, 1);
	zassert_equal(rc, 1, "cannot read file");
	nffs_test_util_assert_cache_range(NFFS_MNTP"/myfile.txt",
						nffs_block_max_data_sz * 3,
						nffs_block_max_data_sz * 4);

	/* Cache second and third blocks. */
	rc = fs_seek(&file, nffs_block_max_data_sz * 1, FS_SEEK_SET);
	zassert_equal(rc, 0, "cannot set pos in file");
	rc = fs_read(&file, &b, 1);
	zassert_equal(rc, 1, "cannot read file");
	nffs_test_util_assert_cache_range(NFFS_MNTP"/myfile.txt",
						nffs_block_max_data_sz * 1,
						nffs_block_max_data_sz * 4);

	/* Cache fifth block. */
	rc = fs_seek(&file, nffs_block_max_data_sz * 4, FS_SEEK_SET);
	zassert_equal(rc, 0, "cannot set pos in file");
	rc = fs_read(&file, &b, 1);
	zassert_equal(rc, 1, "cannot read file");
	nffs_test_util_assert_cache_range(NFFS_MNTP"/myfile.txt",
						nffs_block_max_data_sz * 1,
						nffs_block_max_data_sz * 5);

	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close file");
}
