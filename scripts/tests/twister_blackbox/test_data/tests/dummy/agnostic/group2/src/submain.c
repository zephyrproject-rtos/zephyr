/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>


/**
 * @brief Test Asserts
 *
 * This test verifies various assert macros provided by ztest.
 *
 */
ZTEST(a2_tests, test_assert3)
{
	zassert_true(1, "1 was false");
	zassert_false(0, "0 was true");
	zassert_is_null(NULL, "NULL was not NULL");
	zassert_not_null("foo", "\"foo\" was NULL");
	zassert_equal(1, 1, "1 was not equal to 1");
	zassert_equal_ptr(NULL, NULL, "NULL was not equal to NULL");
}
