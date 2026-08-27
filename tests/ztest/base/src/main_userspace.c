/*
 * Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

ZTEST_USER(framework_tests, test_userspace_is_user)
{
	zassert_true(k_is_user_context());
}
