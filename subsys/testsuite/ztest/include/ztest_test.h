/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Zephyr testing framework _test.
 */

#ifndef __ZTEST_TEST_H__
#define __ZTEST_TEST_H__

#include <app_memory/app_memdomain.h>

struct unit_test {
	const char *name;
	void (*test)(void);
	void (*setup)(void);
	void (*teardown)(void);
	u32_t thread_options;
};

void z_ztest_run_test_suite(const char *name, struct unit_test *suite);

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
 * @brief Pass the currently running test.
 *
 * Normally a test passes just by returning without an assertion failure.
 * However, if the success case for your test involves a fatal fault,
 * you can call this function from z_SysFatalErrorHandler to indicate that
 * the test passed before aborting the thread.
 */
void ztest_test_pass(void);

/**
 * @brief Skip the current test.
 *
 */
void ztest_test_skip(void);

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
		STRINGIFY(fn), fn, setup, teardown, 0 \
}

/**
 * @brief Define a user mode test with setup and teardown functions
 *
 * This should be called as an argument to ztest_test_suite. The test will
 * be run in the following order: @a setup, @a fn, @a teardown. ALL
 * test functions will be run in user mode, and only if CONFIG_USERSPACE
 * is enabled, otherwise this is the same as ztest_unit_test_setup_teardown().
 *
 * @param fn Main test function
 * @param setup Setup function
 * @param teardown Teardown function
 */

#define ztest_user_unit_test_setup_teardown(fn, setup, teardown) { \
		STRINGIFY(fn), fn, setup, teardown, K_USER \
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
 * @brief Define a test function that should run as a user thread
 *
 * This should be called as an argument to ztest_test_suite.
 * If CONFIG_USERSPACE is not enabled, this is functionally identical to
 * ztest_unit_test().
 *
 * @param fn Test function
 */

#define ztest_user_unit_test(fn) \
	ztest_user_unit_test_setup_teardown(fn, unit_test_noop, unit_test_noop)


/* definitions for use with testing application shared memory   */
#ifdef CONFIG_USERSPACE
#define ZTEST_DMEM	K_APP_DMEM(ztest_mem_partition)
#define ZTEST_BMEM	K_APP_BMEM(ztest_mem_partition)
#define ZTEST_SECTION	K_APP_DMEM_SECTION(ztest_mem_partition)
extern struct k_mem_partition ztest_mem_partition;
extern struct k_mem_domain ztest_mem_domain;
#else
#define ZTEST_DMEM
#define ZTEST_BMEM
#define ZTEST_SECTION	.data
#endif

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
 * @param suite Name of the testing suite
 */
#define ztest_test_suite(suite, ...) \
	static ZTEST_DMEM struct unit_test _##suite[] = { \
		__VA_ARGS__, { 0 } \
	}
/**
 * @brief Run the specified test suite.
 *
 * @param suite Test suite to run.
 */
#define ztest_run_test_suite(suite) \
	z_ztest_run_test_suite(#suite, _##suite)

/** @internal
 * @brief Create test function name from base test and index
 */
#define __ztest_func_from_list_name(func, i) test_##func##_##i

/**@internal
 * @brief Create test function with index.
 */
#define __ztest_func_from_list_create(i, func) \
	void __ztest_func_from_list_name(func, i)(void) \
	{ \
		func(i); \
	}

/** @internal
 * @brief Internal macro for declaring single test in test list.
 */
#define __ztest_func_from_list_declare(i, func) \
	ztest_unit_test(__ztest_func_from_list_name(func, i)),


/** @internal
 * @brief Internal macro for installing set of tests.
 *
 * A trick must be used to ensure that last element is without comma and to
 * support single element list.
 */
#define __ztest_unit_tests_from_list(num_less_1, func) \
	COND_CODE_0(num_less_1, \
	  (), \
	  (UTIL_LISTIFY(num_less_1, __ztest_func_from_list_declare, func)) \
	) \
	ztest_unit_test(__ztest_func_from_list_name(func,num_less_1))

/** @internal
 * @brief Internal macro fro creating test functions.
 */
#define __ztest_unit_tests_from_list_create(num_of_devices, func) \
	UTIL_LISTIFY(num_of_devices, __ztest_func_from_list_create, func)

/**
 * @brief Install test spawned based on the list.
 *
 * Following example:
 * ztest_unit_tests_from_list(func, ELEMENT1, ELEMENT2, ELEMENT3,)
 *
 * resolves to:
 * ztest_unit_tests(test_func_0),
 * ztest_unit_tests(test_func_1),
 * ztest_unit_tests(test_func_2)
 *
 * Test names are suffixed with index rather than element name. That is becuase
 * elements can be strings and in that case they cannot form a valid token.
 * Note that last generated element is not terminated with comma.
 *
 * @note list provided as macro argument should be terminated with comma.
 *
 * See @ref ztest_unit_tests_from_list_create for more details.
 */
#define ztest_unit_tests_from_list(func, ...) \
	__ztest_unit_tests_from_list(NUM_VA_ARGS_LESS_2(__VA_ARGS__), func)

/**
 * @brief Spawn test function based on size of the provided list.
 *
 * @note list provided as macro argument should be terminated with comma.
 *
 * Following example:
 * ztest_unit_tests_from_list(func, ELEMENT1, ELEMENT2, ELEMENT3,)
 *
 * resolves to:
 * void test_func_0(void)
 * {
 *	func(0);
 * }
 * void test_func_1(void)
 * {
 *	func(0);
 * }
 * void test_func_2(void)
 * {
 *	func(0);
 * }
 *
 * Test names are suffixed with index rather than element name. That is becuase
 * elements can be strings and in that case they cannot form a valid token.
 *
 * Macro should be used together with @ref ztest_unit_tests_from_list to
 * create set of tests for list of objects (e.g. devices).
 *
 * Example below shows how same test can be created for multiple devices.
 *
 * #define DEVICE_LIST "dev1", "dev2", "dev3",
 * const char *devlist[] = { DEVICE_LIST }
 *
 * void foo(u32_t idx) {
 *	const char *name = devlist[idx];
 *	...
 * }
 *
 * ztest_unit_tests_from_list_create(foo, DEVICE_LIST)
 *
 * void test_main(void)
 * {
 *	ztest_test_suite(test_foo,
 *		ztest_unit_tests_from_list(foo, DEVICE_LIST)
 *	);
 *	ztest_run_test_suite(test_foo);
 *}
 */
#define ztest_unit_tests_from_list_create(func, ...) \
	__ztest_unit_tests_from_list_create(NUM_VA_ARGS_LESS_1(__VA_ARGS__), \
						func)
/**
 * @}
 */

#endif /* __ZTEST_ASSERT_H__ */
