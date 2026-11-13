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

/**
 * @brief littlefs must reject a mount with no pre-allocated fs_data
 */
ZTEST(littlefs, test_fs_mount_null_fs_data)
{
	struct fs_mount_t mp = {
		.type = FS_LITTLEFS,
		.mnt_point = "/nullfs",
		.fs_data = NULL,
	};
	int ret;

	ret = fs_mount(&mp);
	zassert_equal(ret, -EINVAL,
		      "NULL fs_data must be rejected with -EINVAL, got %d", ret);
}
