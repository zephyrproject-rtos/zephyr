/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <pwd.h>

#include <zephyr/ztest.h>

ZTEST(posix_thread_safe_functions, test_pwd_stubs)
{
	zassert_equal(getpwnam_r(NULL, NULL, NULL, 42, NULL), ENOSYS);
	zassert_equal(getpwuid_r(42, NULL, NULL, 42, NULL), ENOSYS);
}
