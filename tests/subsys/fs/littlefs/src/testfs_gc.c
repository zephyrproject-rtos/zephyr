/*
 * Copyright (c) 2025 Trackunit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/fs/littlefs.h>
#include "testfs_tests.h"
#include "testfs_lfs.h"

void test_fs_gc_simple(void);

struct fs_mount_t *fs_gc_mp = &testfs_small_mnt;

static void cleanup(struct fs_mount_t *mp)
{
	TC_PRINT("Clean %s\n", mp->mnt_point);

	zassert_equal(testfs_lfs_wipe_partition(mp), TC_PASS,
		      "Failed to clean partition");
}

ZTEST(littlefs, test_fs_gc_simple_lfs)
{
	cleanup(fs_gc_mp);

	test_fs_gc_simple();
}
