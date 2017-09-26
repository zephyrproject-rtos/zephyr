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
#include <ztest.h>
#include <ztest_assert.h>

void test_gc(void)
{
	int rc;

	static const struct nffs_area_desc area_descs_two[] = {
		{ 0x00000000 + TEST_FLASH_OFFSET, 128 * 1024 },
		{ 0x00020000 + TEST_FLASH_OFFSET, 128 * 1024 },
		{ 0, 0 },
	};

	struct nffs_test_block_desc blocks[8] = { {
		.data = "1",
		.data_len = 1,
	}, {
		.data = "2",
		.data_len = 1,
	}, {
		.data = "3",
		.data_len = 1,
	}, {
		.data = "4",
		.data_len = 1,
	}, {
		.data = "5",
		.data_len = 1,
	}, {
		.data = "6",
		.data_len = 1,
	}, {
		.data = "7",
		.data_len = 1,
	}, {
		.data = "8",
		.data_len = 1,
	} };


	rc = nffs_format_full(area_descs_two);
	zassert_equal(rc, 0, "cannot format nffs");

	nffs_test_util_create_file_blocks("/myfile.txt", blocks, 8);

	nffs_gc(NULL);

	nffs_test_util_assert_block_count("/myfile.txt", 1);
}
