/*
 * Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

struct cpp_fixture {
	int x;
};

void *cpp_setup(void)
{
	auto fixture = new struct cpp_fixture;

	fixture->x = 5;
	return fixture;
}

void cpp_teardown(void *fixture)
{
	delete static_cast<struct cpp_fixture *>(fixture);
}

ZTEST_SUITE(cpp, NULL, cpp_setup, NULL, NULL, cpp_teardown);

ZTEST_F(cpp, test_fixture_created_and_initialized)
{
	zassert_equal(5, fixture->x, NULL);
}
