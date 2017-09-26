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

void test_read(void)
{
	u8_t buf[16];
	fs_file_t file;
	int rc;

	rc = nffs_format_full(nffs_current_area_descs);
	zassert_equal(rc, 0, "cannot format nffs");

	nffs_test_util_create_file("/myfile.txt", "1234567890", 10);

	rc = fs_open(&file, "/myfile.txt");
	zassert_equal(rc, 0, "cannot open file");
	nffs_test_util_assert_file_len(file.fp, 10);
	zassert_equal(fs_tell(&file), 0, "invalid pos in file");

	rc = fs_read(&file, &buf, 4);
	zassert_equal(rc, 4, "invalid bytes read");
	zassert_equal(memcmp(buf, "1234", 4), 0, "invalid buffer size");
	zassert_equal(fs_tell(&file), 4, "invalid pos in file");

	rc = fs_read(&file, buf + 4, sizeof(buf) - 4);
	zassert_equal(rc, 6, "invalid bytes read");
	zassert_equal(memcmp(buf, "1234567890", 10), 0, "invalid buffer size");
	zassert_equal(fs_tell(&file), 10, "invalid pos in file");

	rc = fs_close(&file);
	zassert_equal(rc, 0, "cannot close file");
}
