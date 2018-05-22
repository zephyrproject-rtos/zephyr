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

#define TEST_CONTENT_LEN (1024 * 4)

void test_large_unlink(void)
{
	/* It should not be necessary to initialize this array, but the libgcc
	 * version of strcmp triggers a "Conditional jump or move depends on
	 * uninitialised value(s)" valgrind warning.
	 */
	char filename[256] = { 0 };
	int rc;
	int i;
	int j;
	int k;

	rc = nffs_format_full(nffs_current_area_descs);
	zassert_equal(rc, 0, "cannot format nffs");

	for (i = 0; i < 5; i++) {
		snprintf(filename, sizeof(filename), NFFS_MNTP"/dir0_%d", i);
		rc = fs_mkdir(filename);
		zassert_equal(rc, 0, "cannot create directory");

		for (j = 0; j < 5; j++) {
			snprintf(filename, sizeof(filename),
					NFFS_MNTP"/dir0_%d/dir1_%d", i, j);
			rc = fs_mkdir(filename);
			zassert_equal(rc, 0, "cannot create directory");

			for (k = 0; k < 5; k++) {
				snprintf(filename, sizeof(filename),
					NFFS_MNTP"/dir0_%d/dir1_%d/file2_%d",
					i, j, k);
				nffs_test_util_create_file(filename,
							   nffs_test_buf,
							   TEST_CONTENT_LEN);
			}
		}

		for (j = 0; j < 15; j++) {
			snprintf(filename, sizeof(filename),
					NFFS_MNTP"/dir0_%d/file1_%d", i, j);
			nffs_test_util_create_file(filename, nffs_test_buf,
						   TEST_CONTENT_LEN);
		}
	}

	for (i = 0; i < 5; i++) {
		snprintf(filename, sizeof(filename), NFFS_MNTP"/dir0_%d", i);
		rc = fs_unlink(filename);
		zassert_equal(rc, 0, "cannot unlink file");
	}

	/* The entire file system should be empty. */
	struct nffs_test_file_desc *expected_system =
			(struct nffs_test_file_desc[]) { {
				.filename = "",
				.is_dir = 1,
			} };

	nffs_test_assert_system(expected_system, nffs_current_area_descs);
}
