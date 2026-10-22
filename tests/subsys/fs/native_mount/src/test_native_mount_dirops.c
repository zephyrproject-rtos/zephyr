/*
 * Copyright (c) 2025 Golioth, Inc.
 * Copyright (c) 2025 Marcin Niestroj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdlib.h>
#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>

#include "mount.h"

void test_fs_dirops(void);

/* Mount structure needed by test_fs_basic tests. */
struct fs_mount_t *fs_dirops_test_mp = &test_mp;

ZTEST(native_mount, test_native_mount_dirops)
{
	zassert_equal(fs_mount(&test_mp), 0, "mount failed");
	zassert_equal(fs_rm_all(test_mp.mnt_point), 0, "rm all failed");
	zassert_equal(fs_unmount(&test_mp), 0, "unmount small failed");

	/* Common dirops tests.
	 * (File system is mounted and unmounted during that test.)
	 */
	test_fs_dirops();
}
