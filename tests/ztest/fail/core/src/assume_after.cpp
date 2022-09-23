/* Copyright (c) 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include "fail_test.hpp"

void fail_test_after_impl(void)
{
	zassume_true(false, nullptr);
}

void fail_test_teardown_impl(void) {}
