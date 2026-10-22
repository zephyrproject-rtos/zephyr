/*
 * Copyright (c) 2023 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/ext2.h>

#include "utils.h"

void test_fs_mount_flags(void);

/* Global variable expected by tests */
struct fs_mount_t *mount_flags_mp = &testfs_mnt;

ZTEST(ext2tests, test_mount_flags)
{
	test_fs_mount_flags();
}
