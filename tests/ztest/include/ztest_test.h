/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 *
 * @brief Zephyr testing framework _test.
 */

#ifndef __ZTEST_TEST_H__
#define __ZTEST_TEST_H__

struct unit_test {
	const char *name;
	void (*test)(void);
	void (*setup)(void);
	void (*teardown)(void);
};

void _ztest_run_test_suite(const char *name, struct unit_test *suite);

/**
 * @defgroup ztest_test Ztest testing macros
 * @ingroup ztest
 *
 * This module eases the testing process by providing helpful macros and other
 * testing structures.
 *
 * @{
 */

/**
 * @brief Fail the currently running test.
 *
 * This is the function called from failed assertions and the like. You
 * probably don't need to call it yourself.
 */
void ztest_test_fail(void);

/**
 * @brief Do nothing, successfully.
 *
 * Unit test / setup function / teardown function that does
 * nothing, successfully. Can be used as a parameter to
 * ztest_unit_test_setup_teardown().
 */
static inline void unit_test_noop(void)
{
}

/**
 * @brief Define a test with setup and teardown functions
 *
 * This should be called as an argument to ztest_test_suite. The test will
 * be run in the following order: @a setup, @a fn, @a teardown.
 *
 * @param fn Main test function
 * @param setup Setup function
 * @param teardown Teardown function
 */

#define ztest_unit_test_setup_teardown(fn, setup, teardown) { \
		STRINGIFY(fn), fn, setup, teardown \
}


/**
 * @brief Define a test function
 *
 * This should be called as an argument to ztest_test_suite.
 *
 * @param fn Test function
 */

#define ztest_unit_test(fn) \
	ztest_unit_test_setup_teardown(fn, unit_test_noop, unit_test_noop)

/**
 * @brief Define a test suite
 *
 * This function should be called in the following fashion:
 * ```{.c}
 *      ztest_test_suite(test_suite_name,
 *              ztest_unit_test(test_function),
 *              ztest_unit_test(test_other_function)
 *      );
 *
 *      ztest_run_test_suite(test_suite_name);
 * ```
 *
 * @param name Name of the testing suite
 */
#define ztest_test_suite(name, ...) \
	struct unit_test _##name[] = { \
		__VA_ARGS__, { 0 } \
	}
/**
 * @brief Run the specified test suite.
 *
 * @param suite Test suite to run.
 */
#define ztest_run_test_suite(suite) \
	_ztest_run_test_suite(#suite, _##suite)

/**
 * @}
 */

#endif /* __ZTEST_ASSERT_H__ */
