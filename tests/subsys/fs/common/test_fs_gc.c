/*
 * Copyright (c) 2025 Trackunit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>

/* Mount point and paths must be provided by test runner. */
extern struct fs_mount_t *fs_gc_mp;

void test_fs_gc_simple(void)
{
	int ret = 0;

	TC_PRINT("Mount\n");
	ret = fs_mount(fs_gc_mp);
	zassert_equal(ret, 0, "Expected fs_mount success (%d)", ret);

	TC_PRINT("Try gc\n");
	ret = fs_gc(fs_gc_mp);
	zassert_equal(ret, 0, "Expected fs_gc success (%d)", ret);

	TC_PRINT("Remount filesystem\n");
	ret = fs_unmount(fs_gc_mp);
	zassert_equal(ret, 0, "Expected fs_unmount success (%d)", ret);

	ret = fs_mount(fs_gc_mp);
	zassert_equal(ret, 0, "Expected fs_mount success (%d)", ret);

	ret = fs_unmount(fs_gc_mp);
	zassert_equal(ret, 0, "Expected fs_unmount success (%d)", ret);
}
