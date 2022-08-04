/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

ZTEST_SUITE(framework_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(framework_tests, test_empty_test)
{
}

ZTEST(framework_tests, test_assert_tests)
{
	zassert_true(1, NULL);
	zassert_false(0, NULL);
	zassert_is_null(NULL, NULL);
	zassert_not_null("foo", NULL);
	zassert_equal(1, 1, NULL);
	zassert_equal_ptr(NULL, NULL, NULL);
}

ZTEST(framework_tests, test_assert_mem_equal)
{
	static const uint32_t expected[4] = {
		0x1234,
		0x5678,
		0x9ABC,
		0xDEF0
	};
	uint32_t actual[4] = {0};
	memcpy(actual, expected, sizeof(actual));
	zassert_mem_equal(actual, expected, sizeof(expected), NULL);
}

ZTEST(framework_tests, test_skip_config)
{
	Z_TEST_SKIP_IFDEF(CONFIG_BUGxxxxx);
	ztest_test_fail();
}

ZTEST(framework_tests, test_skip_no_config)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_BUGyyyyy);
	ztest_test_fail();
}

/***************************************************************************************************
 * Sample fixture tests
 **************************************************************************************************/

struct fixture_tests_fixture {
};

static struct fixture_tests_fixture test_fixture;

static void *fixture_tests_setup(void)
{
	return &test_fixture;
}

ZTEST_SUITE(fixture_tests, NULL, fixture_tests_setup, NULL, NULL, NULL);

ZTEST_F(fixture_tests, test_fixture_pointer)
{
	zassert_equal_ptr(&test_fixture, fixture, "Test fixture should be at 0x%x but was at 0x%x",
			  &test_fixture, fixture);
}

/***************************************************************************************************
 * Sample rule tests
 **************************************************************************************************/

enum rule_state {
	RULE_STATE_SETUP = 0,
	RULE_STATE_BEFORE_EACH,
	RULE_STATE_TEST,
	RULE_STATE_AFTER_EACH,
};

struct rules_tests_fixture {
	enum rule_state state;
	int  run_count;
};

static struct rules_tests_fixture rule_tests_fixture;

static void rule_before_each(const struct ztest_unit_test *test, void *data)
{
	if (strcmp(test->test_suite_name, "rules_tests") == 0 &&
	    strcmp(test->name, "test_rules_before_after") == 0) {
		struct rules_tests_fixture *fixture = data;

		zassert_equal_ptr(&rule_tests_fixture, data,
				  "Data expected to point to rule_state");
		if (fixture->run_count == 0) {
			zassert_equal(fixture->state, RULE_STATE_SETUP, "Unexpected state");
		} else {
			zassert_equal(fixture->state, RULE_STATE_AFTER_EACH, "Unexpected state");
		}
		fixture->state = RULE_STATE_BEFORE_EACH;
	}
}

static void rule_after_each(const struct ztest_unit_test *test, void *data)
{
	if (strcmp(test->test_suite_name, "rules_tests") == 0 &&
	    strcmp(test->name, "test_rules_before_after") == 0) {
		struct rules_tests_fixture *fixture = data;

		zassert_equal_ptr(&rule_tests_fixture, data,
				  "Data expected to point to rule_state");
		zassert_equal(fixture->state, RULE_STATE_TEST, "Unexpected state");
		fixture->state = RULE_STATE_AFTER_EACH;
	}
}

static void *rule_test_setup(void)
{
	rule_tests_fixture.state = RULE_STATE_SETUP;
	rule_tests_fixture.run_count = 0;
	return &rule_tests_fixture;
}

static void rule_test_teardown(void *data)
{
	struct rules_tests_fixture *fixture = data;

	/*
	 * Normally, we wouldn't assert here, but it's the only way to test that the rule's
	 * after_each function was called.
	 */
	zassert_equal(fixture->state, RULE_STATE_AFTER_EACH, "Unexpected state");
#ifdef CONFIG_ZTEST_SHUFFLE
	zassert_equal(fixture->run_count, CONFIG_ZTEST_SHUFFLE_TEST_REPEAT_COUNT, NULL);
#endif
}

ZTEST_RULE(verify_before_after_rule, rule_before_each, rule_after_each);

ZTEST_SUITE(rules_tests, NULL, rule_test_setup, NULL, NULL, rule_test_teardown);

ZTEST_F(rules_tests, test_rules_before_after)
{
	zassert_equal(fixture->state, RULE_STATE_BEFORE_EACH,
		      "Unexpected state");
	fixture->state = RULE_STATE_TEST;
	fixture->run_count++;
}
