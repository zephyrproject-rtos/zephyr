/*
 * Copyright 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>

/*
 * Adapted from
 * https://github.com/google/googletest/blob/main/googletest/test/googletest-filter-unittest.py
 */

#define TEST_HACK_IDENTITY_FILTER ((const char *)0x97e57)

/* part of subsys/testsuite/ztest/src/ztest_shell.c */
int ztest_shell_filter(const char *test, const char *filter);

static const char *const PARAM_TESTS[] = {
	"SeqP/ParamTest.TestX/0", "SeqP/ParamTest.TestX/1", "SeqP/ParamTest.TestY/0",
	"SeqP/ParamTest.TestY/1", "SeqQ/ParamTest.TestX/0", "SeqQ/ParamTest.TestX/1",
	"SeqQ/ParamTest.TestY/0", "SeqQ/ParamTest.TestY/1",
};

static const char *const ACTIVE_TESTS[] = {
	"FooTest.Abc",       "FooTest.Xyz",     "BarTest.TestOne", "BarTest.TestTwo",
	"BarTest.TestThree", "BazTest.TestOne", "BazTest.TestA",   "BazTest.TestB",
};

static void run_and_verify(size_t n, const bool *expected, const char *const *test,
			   const char *filter)
{
	bool use_identity = filter == TEST_HACK_IDENTITY_FILTER;

	for (size_t i = 0; i < n; ++i) {
		if (use_identity) {
			filter = test[i];
		}
		zassert_equal(ztest_shell_filter(test[i], filter), expected[i],
			      "expected test %s to be %s with filter %s", test[i],
			      expected[i] ? "run" : "filtered", filter);
	}
}

ZTEST(filter, test_empty)
{
	static const bool expected[] = {
		true, true, true, true, true, true, true, true,
	};
	BUILD_ASSERT(ARRAY_SIZE(expected) == ARRAY_SIZE(PARAM_TESTS));
	BUILD_ASSERT(ARRAY_SIZE(expected) == ARRAY_SIZE(ACTIVE_TESTS));

	run_and_verify(ARRAY_SIZE(PARAM_TESTS), expected, PARAM_TESTS, "");
	run_and_verify(ARRAY_SIZE(ACTIVE_TESTS), expected, ACTIVE_TESTS, NULL);
}

ZTEST(filter, test_bad_filter)
{
	static const bool expected[] = {
		false, false, false, false, false, false, false, false,
	};
	BUILD_ASSERT(ARRAY_SIZE(expected) == ARRAY_SIZE(ACTIVE_TESTS));

	run_and_verify(ARRAY_SIZE(ACTIVE_TESTS), expected, ACTIVE_TESTS, "BadFilter");
}

ZTEST(filter, test_full_name)
{
	static const bool expected[] = {
		true, true, true, true, true, true, true, true,
	};
	BUILD_ASSERT(ARRAY_SIZE(expected) == ARRAY_SIZE(ACTIVE_TESTS));

	/* identity test: filter == test name */
	run_and_verify(ARRAY_SIZE(ACTIVE_TESTS), expected, ACTIVE_TESTS, TEST_HACK_IDENTITY_FILTER);
}

ZTEST(filter, test_universal_filters)
{
	static const bool expected[] = {
		true, true, true, true, true, true, true, true,
	};
	BUILD_ASSERT(ARRAY_SIZE(expected) == ARRAY_SIZE(ACTIVE_TESTS));

	run_and_verify(ARRAY_SIZE(ACTIVE_TESTS), expected, ACTIVE_TESTS, "*");
	run_and_verify(ARRAY_SIZE(ACTIVE_TESTS), expected, ACTIVE_TESTS, "*.*");
}

ZTEST(filter, test_filter_by_suite)
{
	static const bool expected[] = {
		true,  /* FooTest.Abc */
		true,  /* FooTest.Xyz */
		false, /* BarTest.TestOne */
		false, /* BarTest.TestTwo */
		false, /* BarTest.TestThree */
		false, /* BazTest.TestOne */
		false, /* BazTest.TestA */
		false, /* BazTest.TestB */
	};
	BUILD_ASSERT(ARRAY_SIZE(expected) == ARRAY_SIZE(ACTIVE_TESTS));

	run_and_verify(ARRAY_SIZE(ACTIVE_TESTS), expected, ACTIVE_TESTS, "FooTest.*");

	static const bool expected2[] = {
		false, /* FooTest.Abc */
		false, /* FooTest.Xyz */
		false, /* BarTest.TestOne */
		false, /* BarTest.TestTwo */
		false, /* BarTest.TestThree */
		true,  /* BazTest.TestOne */
		true,  /* BazTest.TestA */
		true,  /* BazTest.TestB */
	};
	BUILD_ASSERT(ARRAY_SIZE(expected2) == ARRAY_SIZE(ACTIVE_TESTS));

	run_and_verify(ARRAY_SIZE(ACTIVE_TESTS), expected2, ACTIVE_TESTS, "BazTest.*");
}

ZTEST(filter, test_wildcard_in_suite_name)
{
	static const bool expected[] = {
		false, /* FooTest.Abc */
		false, /* FooTest.Xyz */
		true,  /* BarTest.TestOne */
		true,  /* BarTest.TestTwo */
		true,  /* BarTest.TestThree */
		true,  /* BazTest.TestOne */
		true,  /* BazTest.TestA */
		true,  /* BazTest.TestB */
	};
	BUILD_ASSERT(ARRAY_SIZE(expected) == ARRAY_SIZE(ACTIVE_TESTS));

	run_and_verify(ARRAY_SIZE(ACTIVE_TESTS), expected, ACTIVE_TESTS, "*a*.*");
}

ZTEST(filter, test_wildcard_in_test_name)
{
	static const bool expected[] = {
		true,  /* FooTest.Abc */
		false, /* FooTest.Xyz */
		false, /* BarTest.TestOne */
		false, /* BarTest.TestTwo */
		false, /* BarTest.TestThree */
		false, /* BazTest.TestOne */
		true,  /* BazTest.TestA */
		false, /* BazTest.TestB */
	};
	BUILD_ASSERT(ARRAY_SIZE(expected) == ARRAY_SIZE(ACTIVE_TESTS));

	run_and_verify(ARRAY_SIZE(ACTIVE_TESTS), expected, ACTIVE_TESTS, "*.*A*");
}

ZTEST(filter, test_filter_without_dot)
{
	static const bool expected[] = {
		false, /* FooTest.Abc */
		true,  /* FooTest.Xyz */
		false, /* BarTest.TestOne */
		false, /* BarTest.TestTwo */
		false, /* BarTest.TestThree */
		true,  /* BazTest.TestOne */
		true,  /* BazTest.TestA */
		true,  /* BazTest.TestB */
	};
	BUILD_ASSERT(ARRAY_SIZE(expected) == ARRAY_SIZE(ACTIVE_TESTS));

	run_and_verify(ARRAY_SIZE(ACTIVE_TESTS), expected, ACTIVE_TESTS, "*z*");
}

ZTEST(filter, test_two_patterns)
{
	static const bool expected[] = {
		true,  /* FooTest.Abc */
		true,  /* FooTest.Xyz */
		false, /* BarTest.TestOne */
		false, /* BarTest.TestTwo */
		false, /* BarTest.TestThree */
		false, /* BazTest.TestOne */
		true,  /* BazTest.TestA */
		false, /* BazTest.TestB */
	};
	BUILD_ASSERT(ARRAY_SIZE(expected) == ARRAY_SIZE(ACTIVE_TESTS));

	run_and_verify(ARRAY_SIZE(ACTIVE_TESTS), expected, ACTIVE_TESTS, "Foo*.*:*A*");

	static const bool expected2[] = {
		true,  /* FooTest.Abc */
		false, /* FooTest.Xyz */
		false, /* BarTest.TestOne */
		false, /* BarTest.TestTwo */
		false, /* BarTest.TestThree */
		false, /* BazTest.TestOne */
		true,  /* BazTest.TestA */
		false, /* BazTest.TestB */
	};
	BUILD_ASSERT(ARRAY_SIZE(expected2) == ARRAY_SIZE(ACTIVE_TESTS));

	run_and_verify(ARRAY_SIZE(ACTIVE_TESTS), expected2, ACTIVE_TESTS, ":*A*");
}

ZTEST(filter, test_three_patterns)
{
	static const bool expected[] = {
		true,  /* FooTest.Abc */
		true,  /* FooTest.Xyz */
		true,  /* BarTest.TestOne */
		false, /* BarTest.TestTwo */
		false, /* BarTest.TestThree */
		true,  /* BazTest.TestOne */
		true,  /* BazTest.TestA */
		false, /* BazTest.TestB */
	};
	BUILD_ASSERT(ARRAY_SIZE(expected) == ARRAY_SIZE(ACTIVE_TESTS));

	run_and_verify(ARRAY_SIZE(ACTIVE_TESTS), expected, ACTIVE_TESTS, "*oo*:*A*:*One");

	static const bool expected2[] = {
		true,  /* FooTest.Abc */
		true,  /* FooTest.Xyz */
		true,  /* BarTest.TestOne */
		false, /* BarTest.TestTwo */
		false, /* BarTest.TestThree */
		true,  /* BazTest.TestOne */
		false, /* BazTest.TestA */
		false, /* BazTest.TestB */
	};
	BUILD_ASSERT(ARRAY_SIZE(expected2) == ARRAY_SIZE(ACTIVE_TESTS));

	run_and_verify(ARRAY_SIZE(ACTIVE_TESTS), expected2, ACTIVE_TESTS, "*oo*::*One");

	static const bool expected3[] = {
		true,  /* FooTest.Abc */
		true,  /* FooTest.Xyz */
		false, /* BarTest.TestOne */
		false, /* BarTest.TestTwo */
		false, /* BarTest.TestThree */
		false, /* BazTest.TestOne */
		false, /* BazTest.TestA */
		false, /* BazTest.TestB */
	};
	BUILD_ASSERT(ARRAY_SIZE(expected3) == ARRAY_SIZE(ACTIVE_TESTS));

	run_and_verify(ARRAY_SIZE(ACTIVE_TESTS), expected3, ACTIVE_TESTS, "*oo*::");
}

ZTEST(filter, test_negative_filters)
{
	static const bool expected[] = {
		true,  /* FooTest.Abc */
		true,  /* FooTest.Xyz */
		true,  /* BarTest.TestOne */
		true,  /* BarTest.TestTwo */
		true,  /* BarTest.TestThree */
		false, /* BazTest.TestOne */
		true,  /* BazTest.TestA */
		true,  /* BazTest.TestB */
	};
	BUILD_ASSERT(ARRAY_SIZE(expected) == ARRAY_SIZE(ACTIVE_TESTS));

	run_and_verify(ARRAY_SIZE(ACTIVE_TESTS), expected, ACTIVE_TESTS, "*-BazTest.TestOne");

	static const bool expected2[] = {
		false, /* FooTest.Abc */
		true,  /* FooTest.Xyz */
		true,  /* BarTest.TestOne */
		true,  /* BarTest.TestTwo */
		true,  /* BarTest.TestThree */
		false, /* BazTest.TestOne */
		false, /* BazTest.TestA */
		false, /* BazTest.TestB */
	};
	BUILD_ASSERT(ARRAY_SIZE(expected2) == ARRAY_SIZE(ACTIVE_TESTS));

	run_and_verify(ARRAY_SIZE(ACTIVE_TESTS), expected2, ACTIVE_TESTS,
		       "*-FooTest.Abc:BazTest.*");

	static const bool expected3[] = {
		false, /* FooTest.Abc */
		false, /* FooTest.Xyz */
		true,  /* BarTest.TestOne */
		true,  /* BarTest.TestTwo */
		true,  /* BarTest.TestThree */
		false, /* BazTest.TestOne */
		false, /* BazTest.TestA */
		false, /* BazTest.TestB */
	};
	BUILD_ASSERT(ARRAY_SIZE(expected3) == ARRAY_SIZE(ACTIVE_TESTS));

	run_and_verify(ARRAY_SIZE(ACTIVE_TESTS), expected3, ACTIVE_TESTS,
		       "-FooTest.Abc:FooTest.Xyz:BazTest.*");
}

ZTEST(filter, test_value_parameterized_tests)
{
	static const bool expected[] = {
		true, true, true, true, true, true, true, true,
	};
	BUILD_ASSERT(ARRAY_SIZE(expected) == ARRAY_SIZE(ACTIVE_TESTS));

	run_and_verify(ARRAY_SIZE(PARAM_TESTS), expected, PARAM_TESTS, "*/*");

	static const bool expected2[] = {
		true,  /* SeqP/ParamTest.TestX/0 */
		true,  /* SeqP/ParamTest.TestX/1 */
		true,  /* SeqP/ParamTest.TestY/0 */
		true,  /* SeqP/ParamTest.TestY/1 */
		false, /* SeqQ/ParamTest.TestX/0 */
		false, /* SeqQ/ParamTest.TestX/1 */
		false, /* SeqQ/ParamTest.TestY/0 */
		false, /* SeqQ/ParamTest.TestY/1 */
	};
	BUILD_ASSERT(ARRAY_SIZE(expected2) == ARRAY_SIZE(PARAM_TESTS));

	run_and_verify(ARRAY_SIZE(PARAM_TESTS), expected2, PARAM_TESTS, "SeqP/*");

	static const bool expected3[] = {
		true,  /* SeqP/ParamTest.TestX/0 */
		false, /* SeqP/ParamTest.TestX/1 */
		true,  /* SeqP/ParamTest.TestY/0 */
		false, /* SeqP/ParamTest.TestY/1 */
		true,  /* SeqQ/ParamTest.TestX/0 */
		false, /* SeqQ/ParamTest.TestX/1 */
		true,  /* SeqQ/ParamTest.TestY/0 */
		false, /* SeqQ/ParamTest.TestY/1 */
	};
	BUILD_ASSERT(ARRAY_SIZE(expected3) == ARRAY_SIZE(PARAM_TESTS));

	run_and_verify(ARRAY_SIZE(PARAM_TESTS), expected3, PARAM_TESTS, "*/0");
}

ZTEST_SUITE(filter, NULL, NULL, NULL, NULL, NULL);
