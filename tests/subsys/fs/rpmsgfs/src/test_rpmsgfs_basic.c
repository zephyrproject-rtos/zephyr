/*
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>
#include "test_rpmsgfs.h"

void test_fs_basic(void);

/* Expected by test_fs_basic() */
struct fs_mount_t *fs_basic_test_mp = &testfs_mnt;

ZTEST(rpmsgfs, test_basic)
{
	zassert_equal(fs_mount(fs_basic_test_mp), 0, "mount failed");

	zassert_equal(delete_dir_recursive(fs_basic_test_mp->mnt_point), 0, "wiping mount failed");

	zassert_equal(fs_unmount(fs_basic_test_mp), 0, "mount failed");

	/* Common basic tests.
	 * (File system is mounted and unmounted during that test.)
	 */
	test_fs_basic();
}
