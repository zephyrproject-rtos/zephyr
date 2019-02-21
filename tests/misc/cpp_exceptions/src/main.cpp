/*
 * Copyright (c) 2018 Aurelien Jarno
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This test that C++ exceptions are functional.
 */

#include <zephyr.h>
#include <ztest.h>

#include <exception>


void test_basic_exception(void)
{
	int ret = 0;

	try {
		/* Throwing an integer exception */
		throw 42;
	}
	catch (int e) {
		ret = e;
	}
	catch (...) {
		/* Should not be reached */
		ztest_test_fail();
	}

	zassert_equal(ret, 42, NULL);
}

void test_main(void)
{
	ztest_test_suite(test_cpp_exceptions,
			 ztest_unit_test(test_basic_exception));
	ztest_run_test_suite(test_cpp_exceptions);
}
