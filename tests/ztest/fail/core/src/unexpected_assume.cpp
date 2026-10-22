/* Copyright (c) 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include "fail_test.hpp"

void fail_test_after_impl(void) {}

void fail_test_teardown_impl(void) {}

ZTEST(fail, test_unexpected_assume)
{
	zassume_true(false, NULL);
}
