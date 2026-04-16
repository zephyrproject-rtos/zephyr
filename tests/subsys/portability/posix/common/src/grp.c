/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <grp.h>

#include <zephyr/ztest.h>

ZTEST(grp, test_grp_stubs)
{
	zassert_equal(getgrnam_r(NULL, NULL, NULL, 42, NULL), ENOSYS);
	zassert_equal(getgrgid_r(42, NULL, NULL, 42, NULL), ENOSYS);
}

ZTEST_SUITE(grp, NULL, NULL, NULL, NULL, NULL);
