/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <fs/littlefs.h>
#include "testfs_tests.h"
#include "testfs_lfs.h"

void test_fs_open_flags(void);
/* Expected by test_fs_open_flags() */
const char *test_fs_open_flags_file_path = TESTFS_MNT_POINT_SMALL"/the_file";

static void mount(struct fs_mount_t *mp)
{
	TC_PRINT("Mount %s\n", mp->mnt_point);

	zassert_equal(fs_mount(mp), 0, "Failed to mount partition");
}

static void unmount(struct fs_mount_t *mp)
{
	TC_PRINT("Unmounting %s\n", mp->mnt_point);

	zassert_equal(fs_unmount(mp), 0,
		      "Failed to unmount partition");
}

static void cleanup(struct fs_mount_t *mp)
{
	TC_PRINT("Clean %s\n", mp->mnt_point);

	zassert_equal(testfs_lfs_wipe_partition(mp), TC_PASS,
		      "Failed to clean partition");
}

void test_fs_open_flags_lfs(void)
{
	/* Using smallest partition for this tests as they do not write
	 * a lot of data, basically they just check flags.
	 */
	struct fs_mount_t *mp = &testfs_small_mnt;

	cleanup(mp);
	mount(mp);

	test_fs_open_flags();

	unmount(mp);

}
