/*
 * Copyright (c) 2025 Golioth, Inc.
 * Copyright (c) 2025 Marcin Niestroj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include "mount.h"

void test_fs_open_flags(void);
/* Expected by test_fs_open_flags() */
const char *test_fs_open_flags_file_path = "/test_fs/the_file";

static void mount(struct fs_mount_t *mp)
{
	TC_PRINT("Mount %s\n", mp->mnt_point);

	zassert_equal(fs_mount(mp), 0, "Failed to %s partition", "mount");
}

static void unmount(struct fs_mount_t *mp)
{
	TC_PRINT("Unmounting %s\n", mp->mnt_point);

	zassert_equal(fs_unmount(mp), 0, "Failed to %s partition", "unmount");
}

static void cleanup(struct fs_mount_t *mp)
{
	TC_PRINT("Clean %s\n", mp->mnt_point);

	zassert_equal(fs_rm_all(mp->mnt_point), TC_PASS, "Failed to %s partition", "clean");
}

ZTEST(native_mount, test_native_mount_open_flags)

{
	/* Using smallest partition for this tests as they do not write
	 * a lot of data, basically they just check flags.
	 */
	struct fs_mount_t *mp = &test_mp;

	mp->flags = 0;
	mount(mp);
	cleanup(mp);

	test_fs_open_flags();

	unmount(mp);
}
