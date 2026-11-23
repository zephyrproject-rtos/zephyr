/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

ZTEST(cpp_userspace, test_supervisor_context)
{
	zassert_false(k_is_user_context());
}

ZTEST_USER(cpp_userspace, test_user_context)
{
	zassert_true(k_is_user_context());
}

ZTEST_SUITE(cpp_userspace, NULL, NULL, NULL, NULL, NULL);
