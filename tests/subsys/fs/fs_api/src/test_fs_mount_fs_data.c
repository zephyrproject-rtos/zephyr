/*
 * Copyright (c) 2026 Dhruv Menon <dhruvmenon1104@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_fs.h"

#define MNT_SHORT_A "/dup_a"
#define MNT_SHORT_B "/dup_b"
#define MNT_LONG    "/dup_longer_c"

static int dup_mount(struct fs_mount_t *mountp)
{
	ARG_UNUSED(mountp);
	return 0;
}

static int dup_unmount(struct fs_mount_t *mountp)
{
	ARG_UNUSED(mountp);
	return 0;
}

static const struct fs_file_system_t dup_fs = {
	.mount = dup_mount,
	.unmount = dup_unmount,
};

static struct test_fs_data shared_data;
static struct test_fs_data other_data;

static struct fs_mount_t mnt_short_a_shared = {
		.type = TEST_FS_1,
		.mnt_point = MNT_SHORT_A,
		.fs_data = &shared_data,
};

static struct fs_mount_t mnt_short_b_shared = {
		.type = TEST_FS_1,
		.mnt_point = MNT_SHORT_B,
		.fs_data = &shared_data,
};

static struct fs_mount_t mnt_long_shared = {
		.type = TEST_FS_1,
		.mnt_point = MNT_LONG,
		.fs_data = &shared_data,
};

static struct fs_mount_t mnt_long_other = {
		.type = TEST_FS_1,
		.mnt_point = MNT_LONG,
		.fs_data = &other_data,
};

/* fs_data is intentionally left NULL on the two mount points below. */
static struct fs_mount_t mnt_short_a_null = {
		.type = TEST_FS_1,
		.mnt_point = MNT_SHORT_A,
};

static struct fs_mount_t mnt_long_null = {
		.type = TEST_FS_1,
		.mnt_point = MNT_LONG,
};

/**
 * @brief Sharing one fs_data between two mount points must be rejected
 *
 * @details
 * Covers both mount point length relations, in both directions, because the
 * duplicate detection must not be gated on the mount point lengths.
 *
 * @ingroup filesystem_api
 */
ZTEST(fs_api_mount_fs_data, test_duplicate_fs_data_rejected)
{
	int ret;

	zassert_ok(fs_mount(&mnt_short_a_shared), "failed to mount %s", MNT_SHORT_A);

	/* Mount point of equal length to the mounted one. */
	ret = fs_mount(&mnt_short_b_shared);
	zassert_equal(ret, -EBUSY,
		      "duplicate fs_data accepted for an equal length mount point (%d)", ret);

	/* Mount point longer than the mounted one. */
	ret = fs_mount(&mnt_long_shared);
	zassert_equal(ret, -EBUSY,
		      "duplicate fs_data accepted for a longer mount point (%d)", ret);

	zassert_ok(fs_unmount(&mnt_short_a_shared), "failed to unmount %s", MNT_SHORT_A);

	/* Same collision the other way round, so that the incoming mount point
	 * is shorter than the mounted one.
	 */
	zassert_ok(fs_mount(&mnt_long_shared), "failed to mount %s", MNT_LONG);

	ret = fs_mount(&mnt_short_a_shared);
	zassert_equal(ret, -EBUSY,
		      "duplicate fs_data accepted for a shorter mount point (%d)", ret);

	zassert_ok(fs_unmount(&mnt_long_shared), "failed to unmount %s", MNT_LONG);
}

/**
 * @brief Mount points with distinct fs_data must both be accepted
 *
 * @ingroup filesystem_api
 */
ZTEST(fs_api_mount_fs_data, test_distinct_fs_data_accepted)
{
	zassert_ok(fs_mount(&mnt_short_a_shared), "failed to mount %s", MNT_SHORT_A);
	zassert_ok(fs_mount(&mnt_long_other), "failed to mount %s alongside %s", MNT_LONG,
		   MNT_SHORT_A);

	zassert_ok(fs_unmount(&mnt_long_other), "failed to unmount %s", MNT_LONG);
	zassert_ok(fs_unmount(&mnt_short_a_shared), "failed to unmount %s", MNT_SHORT_A);
}

/**
 * @brief One fs_data may be reused once the previous mount point released it
 *
 * @details
 * The duplicate detection has to reject concurrent owners only; reusing an
 * instance after unmounting has to keep working.
 *
 * @ingroup filesystem_api
 */
ZTEST(fs_api_mount_fs_data, test_fs_data_reuse_after_unmount)
{
	zassert_ok(fs_mount(&mnt_short_a_shared), "failed to mount %s", MNT_SHORT_A);
	zassert_ok(fs_unmount(&mnt_short_a_shared), "failed to unmount %s", MNT_SHORT_A);

	zassert_ok(fs_mount(&mnt_long_shared), "failed to reuse fs_data for %s", MNT_LONG);
	zassert_ok(fs_unmount(&mnt_long_shared), "failed to unmount %s", MNT_LONG);
}

/**
 * @brief A NULL fs_data must not be treated as a duplicate
 *
 * @details
 * File systems such as ext2 allocate their private state inside their own
 * mount handler, so they reach the duplicate check with fs_data still NULL.
 * Two such mount points share no state and must both be accepted.
 *
 * @ingroup filesystem_api
 */
ZTEST(fs_api_mount_fs_data, test_null_fs_data_accepted)
{
	zassert_ok(fs_mount(&mnt_short_a_null), "failed to mount %s with NULL fs_data",
		   MNT_SHORT_A);
	zassert_ok(fs_mount(&mnt_long_null), "NULL fs_data treated as a duplicate for %s",
		   MNT_LONG);

	zassert_ok(fs_unmount(&mnt_long_null), "failed to unmount %s", MNT_LONG);
	zassert_ok(fs_unmount(&mnt_short_a_null), "failed to unmount %s", MNT_SHORT_A);
}

static void *mount_fs_data_setup(void)
{
	zassert_ok(fs_register(TEST_FS_1, &dup_fs), "failed to register test file system");
	return NULL;
}

/* A failing assertion aborts the test in progress, so anything it had already
 * mounted would stay on the mount list and leak into the tests that follow.
 * Drop every mount point after each test to keep the failures isolated.
 */
static void mount_fs_data_after(void *fixture)
{
	static struct fs_mount_t *const all_mnts[] = {
		&mnt_short_a_shared, &mnt_short_b_shared, &mnt_long_shared,
		&mnt_long_other,     &mnt_short_a_null,   &mnt_long_null,
	};

	ARG_UNUSED(fixture);

	ARRAY_FOR_EACH(all_mnts, i) {
		(void)fs_unmount(all_mnts[i]);
	}
}

static void mount_fs_data_teardown(void *fixture)
{
	ARG_UNUSED(fixture);
	(void)fs_unregister(TEST_FS_1, &dup_fs);
}

ZTEST_SUITE(fs_api_mount_fs_data, NULL, mount_fs_data_setup, NULL, mount_fs_data_after,
	    mount_fs_data_teardown);
