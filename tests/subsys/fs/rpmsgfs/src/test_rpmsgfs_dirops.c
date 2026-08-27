/*
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>
#include "test_rpmsgfs.h"

/* Mount structure needed by test_fs_basic tests. */
struct fs_mount_t *fs_dirops_test_mp = &testfs_mnt;

void test_fs_dirops(void);

ZTEST(rpmsgfs, test_dirops)
{
	zassert_equal(fs_mount(fs_dirops_test_mp), 0, "mount failed");

	zassert_equal(delete_dir_recursive(fs_dirops_test_mp->mnt_point), 0, "wiping mount failed");

	zassert_equal(fs_unmount(fs_dirops_test_mp), 0, "unmount failed");

	/* Common dirops tests.
	 * (File system is mounted and unmounted during that test.)
	 */
	test_fs_dirops();
}
