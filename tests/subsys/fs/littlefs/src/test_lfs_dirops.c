/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Directory littlefs operations:
 * * mkdir
 * * opendir
 * * readdir
 * * closedir
 * * rename
 */

#include <string.h>
#include <zephyr/ztest.h>
#include "testfs_tests.h"
#include "testfs_lfs.h"
#include <lfs.h>

#include <zephyr/fs/littlefs.h>

void test_fs_dirops(void);

/* Mount structure needed by test_fs_basic tests. */
struct fs_mount_t *fs_dirops_test_mp = &testfs_small_mnt;

ZTEST(littlefs, test_lfs_dirops)
{
	struct fs_mount_t *mp = &testfs_small_mnt;

	zassert_equal(testfs_lfs_wipe_partition(mp),
		      TC_PASS,
		      "failed to wipe partition");

	/* Common dirops tests.
	 * (File system is mounted and unmounted during that test.)
	 */
	test_fs_dirops();
}
