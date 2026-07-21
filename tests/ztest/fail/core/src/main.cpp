/* Copyright (c) 2022 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include "fail_test.hpp"

static void fail_after(void *) {
	fail_test_after_impl();
}

static void fail_teardown(void *) {
	fail_test_teardown_impl();
}

ZTEST_SUITE(fail, nullptr, nullptr, nullptr, fail_after, fail_teardown);

ZTEST(fail, test_framework) {}
