/*
 * Copyright (c) 2023 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>

#include "utils.h"

void test_fs_dirops(void);

/* Mount structure needed by test_fs_basic tests. */
struct fs_mount_t *fs_dirops_test_mp = &testfs_mnt;

ZTEST(ext2tests, test_dirops)
{
	struct fs_mount_t *mp = &testfs_mnt;
	struct fs_dirent de;

	zassert_equal(fs_mount(mp), 0, "Mount failed");

	/* 'lost+found' directory is created automatically. test_fs_dirops expects empty root
	 * directory hence we have to remove it before starting test.
	 */
	if (fs_stat("/sml/lost+found", &de) == 0) {
		zassert_equal(fs_unlink("/sml/lost+found"), 0, "unlink failed");
	}
	zassert_equal(fs_unmount(mp), 0, "Unount failed");

	/* Common dirops tests.
	 * (File system is mounted and unmounted during that test.)
	 */
	test_fs_dirops();
}
