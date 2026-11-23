/*
 * Copyright (c) 2025 Golioth, Inc.
 * Copyright (c) 2025 Marcin Niestroj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>

#include "mount.h"

void test_fs_basic(void);

struct fs_mount_t *fs_basic_test_mp = &test_mp;

ZTEST(native_mount, test_native_mount_basic)
{
	zassert_equal(fs_mount(&test_mp), 0, "mount failed");
	zassert_equal(fs_rm_all(test_mp.mnt_point), 0, "rm all failed");
	zassert_equal(fs_unmount(&test_mp), 0, "unmount small failed");

	/* Common basic tests.
	 * (File system is mounted and unmounted during that test.)
	 */
	test_fs_basic();
}
