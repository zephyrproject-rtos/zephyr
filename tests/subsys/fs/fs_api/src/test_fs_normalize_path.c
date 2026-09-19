/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_fs.h"
#include <string.h>

struct normalize_case {
	const char *path;
	const char *expected;
};

typedef const char *invalid_path_t;

ZTEST_DEFINE_PARAM_VALUES(invalid_paths, invalid_path_t,
			  NULL,
			  "",
			  "relative/path",
			  "NAND:/foo");

ZTEST_DEFINE_PARAM_VALUES(normalize_cases, struct normalize_case,
			  {"/NAND:", "/NAND:"},
			  {"/NAND:/", "/NAND:"},
			  {"/NAND://foo///bar/", "/NAND:/foo/bar"},
			  {"/NAND:/./foo/./.", "/NAND:/foo"},
			  {"/NAND:/some/path/../here/and/.././my.txt", "/NAND:/some/here/my.txt"},
			  {"/NAND:/..", "/"},
			  {"/NAND:/foo/../..", "/"},
			  {"/..", "/"},
			  {"/", "/"},
			  {"/NAND:/sub/../x", "/NAND:/x"});

ZTEST_P(fs_api_normalize_path, test_normalize_path_invalid)
{
	const char *path = ZTEST_GET_PARAM(invalid_path_t);
	char buf[32];
	int ret;

	ret = fs_normalize_path(path, buf, sizeof(buf));
	zassert_equal(ret, -EINVAL, "\"%s\" should be rejected (%d)", path, ret);
}

ZTEST_P(fs_api_normalize_path, test_normalize_path)
{
	const struct normalize_case *tc = ZTEST_GET_PARAM_PTR(struct normalize_case);
	char buf[64];
	int ret;

	ret = fs_normalize_path(tc->path, buf, sizeof(buf));
	zassert_equal(ret, 0, "\"%s\" should normalize (%d)", tc->path, ret);
	zassert_str_equal(buf, tc->expected, "\"%s\" became \"%s\"", tc->path, buf);
}

ZTEST(fs_api_normalize_path, test_normalize_path_invalid_buffer)
{
	char buf[32];
	int ret;

	ret = fs_normalize_path("/NAND:/foo", NULL, sizeof(buf));
	zassert_equal(ret, -EINVAL, "NULL output buffer should be rejected (%d)", ret);

	ret = fs_normalize_path("/NAND:/foo", buf, 0);
	zassert_equal(ret, -EINVAL, "zero-length output buffer should be rejected (%d)", ret);
}

ZTEST(fs_api_normalize_path, test_normalize_path_buffer_too_small)
{
	char buf[16];
	int ret;

	ret = fs_normalize_path("/NAND:/foo", buf, strlen("/NAND:/foo"));
	zassert_equal(ret, -ENAMETOOLONG, "undersized buffer should be rejected (%d)", ret);

	ret = fs_normalize_path("/NAND:/foo", buf, strlen("/NAND:/foo") + 1);
	zassert_equal(ret, 0, "exactly-sized buffer should succeed (%d)", ret);
	zassert_str_equal(buf, "/NAND:/foo", "unexpected result");

	ret = fs_normalize_path("/NAND:/..", buf, 1);
	zassert_equal(ret, -ENAMETOOLONG, "no room for \"/\" should be rejected (%d)", ret);

	/* The check is per segment, so a prefix a later ".." pops is charged for */
	ret = fs_normalize_path("/a/..", buf, sizeof("/"));
	zassert_equal(ret, -ENAMETOOLONG, "a prefix that does not fit should be rejected (%d)",
		      ret);
}

ZTEST(fs_api_normalize_path, test_normalize_path_in_place)
{
	char buf[64];
	int ret;

	strcpy(buf, "/NAND:/some/path/../here/and/.././my.txt");

	ret = fs_normalize_path(buf, buf, sizeof(buf));

	zassert_equal(ret, 0, "unexpected error (%d)", ret);
	zassert_str_equal(buf, "/NAND:/some/here/my.txt", "unexpected result");
}

ZTEST_SUITE(fs_api_normalize_path, NULL, NULL, NULL, NULL, NULL);

ZTEST_INSTANTIATE_TEST_SUITE_P(all, fs_api_normalize_path, test_normalize_path_invalid,
			       invalid_paths);
ZTEST_INSTANTIATE_TEST_SUITE_P(all, fs_api_normalize_path, test_normalize_path,
			       normalize_cases);
