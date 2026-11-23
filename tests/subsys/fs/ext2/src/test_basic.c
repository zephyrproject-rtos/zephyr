/*
 * Copyright (c) 2023 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>

#include "utils.h"

void test_fs_basic(void);

/* Expected by test_fs_basic() */
struct fs_mount_t *fs_basic_test_mp = &testfs_mnt;

ZTEST(ext2tests, test_basic)
{
	/* Common basic tests.
	 * (File system is mounted and unmounted during that test.)
	 */
	test_fs_basic();
}
