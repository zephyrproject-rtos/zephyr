/*
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>
#include "test_rpmsgfs.h"

void test_fs_open_flags(void);
/* Expected by test_fs_open_flags() */
const char *test_fs_open_flags_file_path = "/sml/the_file";

ZTEST(rpmsgfs, test_fs_open_flags)
{
	struct fs_mount_t *mp = &testfs_mnt;

	zassert_equal(fs_mount(mp), 0, "mount failed");

	test_fs_open_flags();
}
