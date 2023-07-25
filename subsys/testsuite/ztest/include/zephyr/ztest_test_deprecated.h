/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Zephyr testing framework _test_deprecated.
 */

#ifndef ZEPHYR_TESTSUITE_ZTEST_TEST_H_
#define ZEPHYR_TESTSUITE_ZTEST_TEST_H_

#include <zephyr/app_memory/app_memdomain.h>
#include <zephyr/init.h>
#include <zephyr/sys/iterable_sections.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct unit_test {
	const char *name;
	void (*test)(void);
	void (*setup)(void);
	void (*teardown)(void);
	uint32_t thread_options;
};

/**
 * Stats about a ztest suite
 */
struct ztest_suite_stats {
	/** The number of times that the suite ran */
	uint32_t run_count;
	/** The number of times that the suite was skipped */
	uint32_t skip_count;
	/** The number of times that the suite failed */
	uint32_t fail_count;
};

/**
 * A single node of test suite. Each node should be added to a single linker section which will
 * allow ztest_run_registered_test_suites() to iterate over the various nodes.
 */
struct ztest_suite_node {
	/** The name of the test suite. */
	const char *name;
	/** Pointer to the test suite. */
	struct unit_test *suite;
	/**
	 * An optional predicate function to determine if the test should run. If NULL, then the
	 * test will only run once on the first attempt.
	 *
	 * @param state The current state of the test application.
	 * @return True if the suite should be run; false to skip.
	 */
	bool (*predicate)(const void *state);
	/** Stats */
	struct ztest_suite_stats *stats;
};

extern struct ztest_suite_node _ztest_suite_node_list_start[];
extern struct ztest_suite_node _ztest_suite_node_list_end[];

/**
 * Create and register a ztest suite. Using this macro creates a new test suite (using
 * ztest_test_suite). It then creates a struct ztest_suite_node in a specific linker section.
 *
 * Tests can then be run by calling ztest_run_registered_test_suites(const void *state) by passing
 * in the current state. See the documentation for ztest_run_registered_test_suites for more info.
 *
 * @param SUITE_NAME The name of the suite (see ztest_test_suite for more info)
 * @param PREDICATE A function to test against the state and determine if the test should run.
 * @param args Varargs placeholder for the remaining arguments passed for the unit tests.
 */
#define ztest_register_test_suite(SUITE_NAME, PREDICATE, args...)                                  \
	ztest_test_suite(SUITE_NAME, ##args);                                                      \
	struct ztest_suite_stats UTIL_CAT(z_ztest_test_node_stats_, SUITE_NAME);                   \
	static STRUCT_SECTION_ITERABLE(ztest_suite_node, z_ztest_test_node_##SUITE_NAME) = {       \
		.name = #SUITE_NAME,                                                               \
		.suite = _##SUITE_NAME,                                                            \
		.predicate = PREDICATE,                                                            \
		.stats = &UTIL_CAT(z_ztest_test_node_stats_, SUITE_NAME),                          \
	};

/**
 * Run the registered unit tests which return true from their pragma function.
 *
 * @param state The current state of the machine as it relates to the test executable.
 * @return The number of tests that ran.
 */
__deprecated
int ztest_run_registered_test_suites(const void *state);

/**
 * @brief Fails the test if any of the registered tests did not run.
 *
 * When registering test suites, a pragma function can be provided to determine WHEN the test should
 * run. It is possible that a test suite could be registered but the pragma always prevents it from
 * running. In cases where a test should make sure that ALL suites ran at least once, this function
 * may be called at the end of test_main(). It will cause the test to fail if any suite was
 * registered but never ran.
 */
__deprecated
void ztest_verify_all_registered_test_suites_ran(void);

/**
 * @brief Run a test suite.
 *
 * Internal implementation. Do not call directly. This will run the full test suite along with some
 * checks for fast failures and initialization.
 *
 * @param name The name of the suite to run.
 * @param suite Pointer to the first unit test.
 * @return Negative value if the test suite never ran; otherwise, return the number of failures.
 */
int z_ztest_run_test_suite(const char *name, struct unit_test *suite);

/**
 * @defgroup ztest_test_deprecated Ztest testing macros
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
 * you can call this function from k_sys_fatal_error_handler to indicate that
 * the test passed before aborting the thread.
 */
void ztest_test_pass(void);

/**
 * @brief Skip the current test.
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
#define ztest_unit_test_setup_teardown(fn, setup, teardown)                                        \
	{                                                                                          \
		STRINGIFY(fn), fn, setup, teardown, 0                                              \
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
#define ztest_user_unit_test_setup_teardown(fn, setup, teardown)                                   \
	{                                                                                          \
		STRINGIFY(fn), fn, setup, teardown, K_USER                                         \
	}

/**
 * @brief Define a test function
 *
 * This should be called as an argument to ztest_test_suite.
 *
 * @param fn Test function
 */
#define ztest_unit_test(fn)                                                                        \
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
#define ztest_user_unit_test(fn)                                                                   \
	ztest_user_unit_test_setup_teardown(fn, unit_test_noop, unit_test_noop)

/**
 * @brief Define a SMP-unsafe test function
 *
 * As ztest_unit_test(), but ensures all test code runs on only
 * one CPU when in SMP.
 *
 * @param fn Test function
 */
#ifdef CONFIG_SMP
#define ztest_1cpu_unit_test(fn)                                                                   \
	ztest_unit_test_setup_teardown(fn, z_test_1cpu_start, z_test_1cpu_stop)
#else
#define ztest_1cpu_unit_test(fn) ztest_unit_test(fn)
#endif

/**
 * @brief Define a SMP-unsafe test function that should run as a user thread
 *
 * As ztest_user_unit_test(), but ensures all test code runs on only
 * one CPU when in SMP.
 *
 * @param fn Test function
 */
#ifdef CONFIG_SMP
#define ztest_1cpu_user_unit_test(fn)                                                              \
	ztest_user_unit_test_setup_teardown(fn, z_test_1cpu_start, z_test_1cpu_stop)
#else
#define ztest_1cpu_user_unit_test(fn) ztest_user_unit_test(fn)
#endif

/* definitions for use with testing application shared memory   */
#ifdef CONFIG_USERSPACE
#define ZTEST_DMEM	K_APP_DMEM(ztest_mem_partition)
#define ZTEST_BMEM	K_APP_BMEM(ztest_mem_partition)
#define ZTEST_SECTION	K_APP_DMEM_SECTION(ztest_mem_partition)
extern struct k_mem_partition ztest_mem_partition;
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
#define ztest_test_suite(suite, ...) __DEPRECATED_MACRO                                            \
	static ZTEST_DMEM struct unit_test _##suite[] = { __VA_ARGS__, { 0 } }
/**
 * @brief Run the specified test suite.
 *
 * @param suite Test suite to run.
 */
#define ztest_run_test_suite(suite)  __DEPRECATED_MACRO                                            \
	z_ztest_run_test_suite(#suite, _##suite)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_TESTSUITE_ZTEST_TEST_H_ */
