/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/fs/littlefs.h>
#include "testfs_tests.h"
#include "testfs_lfs.h"

void test_fs_mount_flags(void);
/* Using smallest partition for this tests as they do not write
 * a lot of data, basically they just check flags.
 */
struct fs_mount_t *mount_flags_mp = &testfs_small_mnt;

static void cleanup(struct fs_mount_t *mp)
{
	TC_PRINT("Clean %s\n", mp->mnt_point);

	zassert_equal(testfs_lfs_wipe_partition(mp), TC_PASS,
		      "Failed to clean partition");
}

ZTEST(littlefs, test_fs_mount_flags_lfs)
{
	cleanup(mount_flags_mp);

	test_fs_mount_flags();
}
