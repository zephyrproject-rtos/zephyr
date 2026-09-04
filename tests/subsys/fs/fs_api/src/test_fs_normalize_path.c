/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_fs.h"
#include <string.h>

static struct test_fs_data test_data;
static struct fs_mount_t test_fs_mnt = {
	.type = TEST_FS_1,
	.mnt_point = TEST_FS_MNTP,
	.fs_data = &test_data,
};

static void *fs_normalize_path_setup(void)
{
	fs_register(TEST_FS_1, &temp_fs);
	fs_mount(&test_fs_mnt);
	return NULL;
}

static void fs_normalize_path_teardown(void *fixture)
{
	fs_unmount(&test_fs_mnt);
	fs_unregister(TEST_FS_1, &temp_fs);
}

ZTEST(fs_api_normalize_path, test_normalize_path_invalid_args)
{
	char buf[32];
	int ret;

	ret = fs_normalize_path(NULL, buf, sizeof(buf));
	zassert_equal(ret, -EINVAL, "NULL path should be rejected (%d)", ret);

	ret = fs_normalize_path("relative/path", buf, sizeof(buf));
	zassert_equal(ret, -EINVAL, "path without leading / should be rejected (%d)", ret);

	ret = fs_normalize_path("", buf, sizeof(buf));
	zassert_equal(ret, -EINVAL, "empty path should be rejected (%d)", ret);

	ret = fs_normalize_path(TEST_FS_MNTP "/foo", NULL, sizeof(buf));
	zassert_equal(ret, -EINVAL, "NULL output buffer should be rejected (%d)", ret);

	ret = fs_normalize_path(TEST_FS_MNTP "/foo", buf, 0);
	zassert_equal(ret, -EINVAL, "zero-length output buffer should be rejected (%d)", ret);

	ret = fs_normalize_path("/NOSUCH:/foo", buf, sizeof(buf));
	zassert_equal(ret, -ENOENT, "path with no known mount point should be rejected (%d)", ret);
}

ZTEST(fs_api_normalize_path, test_normalize_path_mount_point_only)
{
	char buf[32];
	int ret;

	ret = fs_normalize_path(TEST_FS_MNTP, buf, sizeof(buf));
	zassert_equal(ret, 0, "mount point alone should normalize (%d)", ret);
	zassert_str_equal(buf, TEST_FS_MNTP, "mount point should be unchanged");

	ret = fs_normalize_path(TEST_FS_MNTP "/", buf, sizeof(buf));
	zassert_equal(ret, 0, "mount point with trailing / should normalize (%d)", ret);
	zassert_str_equal(buf, TEST_FS_MNTP, "trailing / on mount point should be dropped");
}

ZTEST(fs_api_normalize_path, test_normalize_path_dot_components)
{
	char buf[32];
	int ret;

	ret = fs_normalize_path(TEST_FS_MNTP "/./foo/./.", buf, sizeof(buf));
	zassert_equal(ret, 0, "run of . components should normalize (%d)", ret);
	zassert_str_equal(buf, TEST_FS_MNTP "/foo", "unexpected result");
}

ZTEST(fs_api_normalize_path, test_normalize_path_slashes)
{
	char buf[32];
	int ret;

	ret = fs_normalize_path(TEST_FS_MNTP "//foo///bar/", buf, sizeof(buf));
	zassert_equal(ret, 0, "repeated and trailing / should normalize (%d)", ret);
	zassert_str_equal(buf, TEST_FS_MNTP "/foo/bar", "unexpected result");
}

ZTEST(fs_api_normalize_path, test_normalize_path_dotdot)
{
	char buf[64];
	int ret;

	ret = fs_normalize_path(TEST_FS_MNTP "/some/path/../here/and/.././my.txt", buf,
				sizeof(buf));
	zassert_equal(ret, 0, "unexpected error (%d)", ret);
	zassert_str_equal(buf, TEST_FS_MNTP "/some/here/my.txt", "unexpected result");
}

ZTEST(fs_api_normalize_path, test_normalize_path_dotdot_past_root)
{
	char buf[32];
	int ret;

	ret = fs_normalize_path(TEST_FS_MNTP "/..", buf, sizeof(buf));
	zassert_equal(ret, -EINVAL, ".. past the mount point root should be rejected (%d)", ret);

	ret = fs_normalize_path(TEST_FS_MNTP "/foo/../..", buf, sizeof(buf));
	zassert_equal(ret, -EINVAL, ".. past the mount point root should be rejected (%d)", ret);
}

ZTEST(fs_api_normalize_path, test_normalize_path_buffer_too_small)
{
	char buf[16];
	int ret;

	zassert_equal(strlen(TEST_FS_MNTP "/foo"), 10, "test assumption changed");

	ret = fs_normalize_path(TEST_FS_MNTP "/foo", buf, 10);
	zassert_equal(ret, -ENAMETOOLONG, "undersized buffer should be rejected (%d)", ret);

	ret = fs_normalize_path(TEST_FS_MNTP "/foo", buf, 11);
	zassert_equal(ret, 0, "exactly-sized buffer should succeed (%d)", ret);
	zassert_str_equal(buf, TEST_FS_MNTP "/foo", "unexpected result");
}

static int root_mount(struct fs_mount_t *mountp)
{
	ARG_UNUSED(mountp);
	return 0;
}

static int root_unmount(struct fs_mount_t *mountp)
{
	ARG_UNUSED(mountp);
	return 0;
}

static const struct fs_file_system_t root_fs = {
	.mount = root_mount,
	.unmount = root_unmount,
};

ZTEST(fs_api_normalize_path, test_normalize_path_root_mount_point)
{
	static struct test_fs_data root_data;
	static struct fs_mount_t root_mnt = {
		.type = TEST_FS_2,
		.mnt_point = "/",
		.fs_data = &root_data,
	};
	char buf[32];
	int ret;

	zassert_ok(fs_register(TEST_FS_2, &root_fs), "register root fs");
	zassert_ok(fs_mount(&root_mnt), "mount at the root");

	ret = fs_normalize_path("/foo//./bar", buf, sizeof(buf));
	zassert_equal(ret, 0, "normalize under a root mount point (%d)", ret);
	zassert_str_equal(buf, "/foo/bar", "got \"%s\"", buf);

	ret = fs_normalize_path("/foo/..", buf, sizeof(buf));
	zassert_equal(ret, 0, "climbing back to the root (%d)", ret);
	zassert_str_equal(buf, "/", "got \"%s\"", buf);

	ret = fs_normalize_path("/", buf, sizeof(buf));
	zassert_equal(ret, 0, "the root itself (%d)", ret);
	zassert_str_equal(buf, "/", "got \"%s\"", buf);

	ret = fs_normalize_path("/..", buf, sizeof(buf));
	zassert_equal(ret, -EINVAL, ".. above the root should be rejected (%d)", ret);

	zassert_ok(fs_unmount(&root_mnt), "unmount the root");
	fs_unregister(TEST_FS_2, &root_fs);
}

ZTEST(fs_api_normalize_path, test_normalize_path_in_place)
{
	char buf[64];

	strcpy(buf, TEST_FS_MNTP "/some/path/../here/and/.././my.txt");

	int ret = fs_normalize_path(buf, buf, sizeof(buf));

	zassert_equal(ret, 0, "unexpected error (%d)", ret);
	zassert_str_equal(buf, TEST_FS_MNTP "/some/here/my.txt", "unexpected result");
}

ZTEST_SUITE(fs_api_normalize_path, NULL, fs_normalize_path_setup, NULL, NULL,
	    fs_normalize_path_teardown);
