/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Tests of functions in testfs_util */

#include <string.h>
#include <ztest.h>
#include "testfs_tests.h"
#include "testfs_util.h"

#define MNT "/mnt"
#define ELT1 "a"
#define ELT2 "b"

static struct testfs_path path;
static const struct fs_mount_t mnt = {
	.mnt_point = MNT,
};

static inline struct testfs_path *reset_path()
{
	testfs_path_init(&path, &mnt, TESTFS_PATH_END);
	return &path;
}

void test_util_path_init_base(void)
{
	zassert_equal(testfs_path_init(&path, NULL, TESTFS_PATH_END),
		      path.path,
		      "bad root init return");
	zassert_equal(strcmp(path.path, "/"), 0, "bad root init path");

	zassert_equal(testfs_path_init(&path, &mnt, TESTFS_PATH_END),
		      path.path,
		      "bad mnt init return");
	zassert_equal(strcmp(path.path, mnt.mnt_point), 0, "bad mnt init path");

	if (IS_ENABLED(CONFIG_DEBUG)) {
		struct fs_mount_t invalid = {
			.mnt_point = "relative",
		};

		/* Apparently no way to verify this without panic. */
		testfs_path_init(&path, &invalid, TESTFS_PATH_END);
	}
}

void test_util_path_init_overrun(void)
{
	char overrun[TESTFS_PATH_MAX + 2] = "/";
	size_t overrun_max = sizeof(overrun) - 1;
	size_t path_max = sizeof(path.path) - 1;
	struct fs_mount_t overrun_mnt = {
		.mnt_point = overrun,
	};

	zassert_true(path_max < overrun_max,
		     "path length requirements unmet");

	memset(overrun + 1, 'A', overrun_max - 1);

	zassert_equal(testfs_path_init(&path, &overrun_mnt, TESTFS_PATH_END),
		      path.path,
		      "bad overrun init return");
	zassert_true(strcmp(path.path, overrun) < 0,
		     "bad overrun init");
	zassert_equal(strncmp(path.path, overrun, path_max),
		      0,
		      "bad overrun path");
	zassert_equal(path.path[path_max], '\0',
		      "missing overrun EOS");
}

void test_util_path_init_extended(void)
{
	zassert_equal(strcmp(testfs_path_init(&path, &mnt,
					      ELT1,
					      TESTFS_PATH_END),
			     MNT "/" ELT1),
		      0,
		      "bad mnt init elt1");

	zassert_equal(strcmp(testfs_path_init(&path, &mnt,
					      ELT1,
					      ELT2,
					      TESTFS_PATH_END),
			     MNT "/" ELT1 "/" ELT2),
		      0,
		      "bad mnt init elt1 elt2");
}

void test_util_path_extend(void)
{
	zassert_equal(strcmp(testfs_path_extend(reset_path(),
						TESTFS_PATH_END),
			     MNT),
		      0,
		      "empty extend failed");

	zassert_equal(strcmp(testfs_path_extend(reset_path(),
						ELT2,
						TESTFS_PATH_END),
			     MNT "/" ELT2),
		      0,
		      "elt extend failed");

	zassert_equal(strcmp(testfs_path_extend(reset_path(),
						ELT1,
						ELT2,
						TESTFS_PATH_END),
			     MNT "/" ELT1 "/" ELT2),
		      0,
		      "elt1 elt2 extend failed");
}

void test_util_path_extend_up(void)
{
	zassert_equal(strcmp(testfs_path_extend(reset_path(),
						ELT2,
						"..",
						ELT1,
						TESTFS_PATH_END),
			     MNT "/" ELT1),
		      0,
		      "elt elt2, up, elt1 failed");

	zassert_equal(strcmp(testfs_path_extend(reset_path(),
						"..",
						TESTFS_PATH_END),
			     "/"),
		      0,
		      "up strip mnt failed");

	zassert_equal(strcmp(testfs_path_extend(reset_path(),
						"..",
						"..",
						TESTFS_PATH_END),
			     "/"),
		      0,
		      "up from root failed");
}

void test_util_path_extend_overrun(void)
{
	char long_elt[TESTFS_PATH_MAX];

	memset(long_elt, 'a', sizeof(long_elt) - 1);
	long_elt[sizeof(long_elt) - 1] = '\0';

	zassert_equal(strcmp(testfs_path_extend(reset_path(),
						long_elt,
						ELT1,
						TESTFS_PATH_END),
			     MNT),
		      0,
		      "stop at overrun failed");
}
