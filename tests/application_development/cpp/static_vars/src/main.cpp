/*
 * Copyright (c) 2019 Erwin Rol <erwin@erwinrol.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

BUILD_ASSERT(__cplusplus == 201703);

static int func_trivial_static(void)
{
	// one time initialization to 42
	static int counter{42};

	// every time this function is called it will increase counter
	counter++;

	return counter;
}

static void test_pod_static_initializer(void)
{
	// the first time func_trivial static will initialize
	// its static counter variable to 42 and than directly
	// increase it by 1
	int i = func_trivial_static();

	zassert_equal(i, 43, "static initializer failed");

	// the second time the function is called it will not do
	// the initialization and only increase its counter
	// variable by 1
	i = func_trivial_static();

	zassert_equal(i, 44, "static initializer failed");
}

// Helper struct with custom constructor to force it to
// be non-trivial
struct CounterHelper
{
	CounterHelper() {}
	CounterHelper(int i) : counter_(i) {}

	int counter_{42};
};

static int func_non_trivial_static(void)
{
	// one time static initialization of counter_helper
	// Since the default constructor is used the counter_
	// member variable will be set to 42
	static CounterHelper counter_helper;

	// every time this function is called it will increase counter_
	counter_helper.counter_++;

	return counter_helper.counter_;
}

static void test_static_initializer(void)
{
	// with the first call to func_non_trivial_static the constructor
	// of counter_helper will set the counter_ member variable
	// to 42 and directly after that increase it by one
	int i = func_non_trivial_static();

	zassert_equal(i, 43, "static initializer failed");

	// with the second call it will only increase the counter_ member
	// variable by one
	i = func_non_trivial_static();

	zassert_equal(i, 44, "static initializer failed");
}

static int func_non_trivial_static_with_value(void)
{
	// one time static initialization of counter_helper
	// using the constructor that takes an int, resulting
	// the counter_ member variable to be set to 13
	static CounterHelper counter_helper(13);

	// every time this function is called it will increase counter_
	counter_helper.counter_++;

	return counter_helper.counter_;
}

static void test_static_with_value_initializer(void)
{
	// with the first call to func_non_trivial_static_with_value the
	// constructor of counter_helper will set the counter_ member variable
	// to 13 and directly after that increase it by one
	int i = func_non_trivial_static_with_value();

	zassert_equal(i, 14, "static initializer failed");

	// with the second call it will only increase the counter_ member
	// variable by one
	i = func_non_trivial_static_with_value();

	zassert_equal(i, 15, "static initializer failed");
}

void test_main(void)
{
	TC_PRINT("version %u\n", (uint32_t)__cplusplus);
	ztest_test_suite(cpp_static_vars_tests,
			 ztest_unit_test(test_pod_static_initializer),
			 ztest_unit_test(test_static_initializer),
			 ztest_unit_test(test_static_with_value_initializer)
		);

	ztest_run_test_suite(cpp_static_vars_tests);
}
