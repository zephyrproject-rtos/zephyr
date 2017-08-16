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

void test_many_children(void)
{
	int rc;

	/*** Setup. */
	rc = nffs_format_full(nffs_current_area_descs);
	zassert_equal(rc, 0, "cannot format nffs");

	nffs_test_util_create_file("/zasdf", NULL, 0);
	nffs_test_util_create_file("/FfD", NULL, 0);
	nffs_test_util_create_file("/4Zvv", NULL, 0);
	nffs_test_util_create_file("/*(*2fs", NULL, 0);
	nffs_test_util_create_file("/pzzd", NULL, 0);
	nffs_test_util_create_file("/zasdf0", NULL, 0);
	nffs_test_util_create_file("/23132.bin", NULL, 0);
	nffs_test_util_create_file("/asldkfjaldskfadsfsdf.txt", NULL, 0);
	nffs_test_util_create_file("/sdgaf", NULL, 0);
	nffs_test_util_create_file("/939302**", NULL, 0);
	rc = fs_mkdir("/dir");
	nffs_test_util_create_file("/dir/itw82", NULL, 0);
	nffs_test_util_create_file("/dir/124", NULL, 0);

	struct nffs_test_file_desc *expected_system =
		(struct nffs_test_file_desc[]) { {
			.filename = "",
			.is_dir = 1,
			.children = (struct nffs_test_file_desc[]) {
				{ "zasdf" },
				{ "FfD" },
				{ "4Zvv" },
				{ "*(*2fs" },
				{ "pzzd" },
				{ "zasdf0" },
				{ "23132.bin" },
				{ "asldkfjaldskfadsfsdf.txt" },
				{ "sdgaf" },
				{ "939302**" },
				{
					.filename = "dir",
					.is_dir = 1,
					.children = (struct nffs_test_file_desc[]) {
						{ "itw82" },
						{ "124" },
						{ NULL },
					},
				},
				{ NULL },
			}
		} };

	nffs_test_assert_system(expected_system, nffs_current_area_descs);
}
