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

#ifndef ZEPHYR_TESTSUITE_ZTEST_TEST_H_
#define ZEPHYR_TESTSUITE_ZTEST_TEST_H_

#include <zephyr/app_memory/app_memdomain.h>
#include <zephyr/init.h>
#include <stdbool.h>

#if defined(CONFIG_USERSPACE)
#define __USERSPACE_FLAGS (K_USER)
#else
#define __USERSPACE_FLAGS (0)
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct ztest_unit_test {
	const char *test_suite_name;
	const char *name;
	void (*test)(void *data);
	uint32_t thread_options;
};

extern struct ztest_unit_test _ztest_unit_test_list_start[];
extern struct ztest_unit_test _ztest_unit_test_list_end[];
#define ZTEST_TEST_COUNT (_ztest_unit_test_list_end - _ztest_unit_test_list_start)

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
 * allow ztest_run_test_suites() to iterate over the various nodes.
 */
struct ztest_suite_node {
	/** The name of the test suite. */
	const char *const name;
	/**
	 * Setup function to run before running this suite
	 *
	 * @return Pointer to the data structure that will be used throughout this test suite
	 */
	void *(*const setup)(void);
	/**
	 * Function to run before each test in this suite
	 *
	 * @param data The test suite's data returned from setup()
	 */
	void (*const before)(void *data);
	/**
	 * Function to run after each test in this suite
	 *
	 * @param data The test suite's data returned from setup()
	 */
	void (*const after)(void *data);
	/**
	 * Teardown function to run after running this suite
	 *
	 * @param data The test suite's data returned from setup()
	 */
	void (*const teardown)(void *data);
	/**
	 * An optional predicate function to determine if the test should run. If NULL, then the
	 * test will only run once on the first attempt.
	 *
	 * @param state The current state of the test application.
	 * @return True if the suite should be run; false to skip.
	 */
	bool (*const predicate)(const void *state);
	/** Stats */
	struct ztest_suite_stats *const stats;
};

extern struct ztest_suite_node _ztest_suite_node_list_start[];
extern struct ztest_suite_node _ztest_suite_node_list_end[];
#define ZTEST_SUITE_COUNT (_ztest_suite_node_list_end - _ztest_suite_node_list_start)

/**
 * Create and register a ztest suite. Using this macro creates a new test suite (using
 * ztest_test_suite). It then creates a struct ztest_suite_node in a specific linker section.
 *
 * Tests can then be run by calling ztest_run_test_suites(const void *state) by passing
 * in the current state. See the documentation for ztest_run_test_suites for more info.
 *
 * @param SUITE_NAME The name of the suite (see ztest_test_suite for more info)
 * @param PREDICATE A function to test against the state and determine if the test should run.
 * @param setup_fn The setup function to call before running this test suite
 * @param before_fn The function to call before each unit test in this suite
 * @param after_fn The function to call after each unit test in this suite
 * @param teardown_fn The function to call after running all the tests in this suite
 */
#define ZTEST_SUITE(SUITE_NAME, PREDICATE, setup_fn, before_fn, after_fn, teardown_fn)             \
	struct ztest_suite_stats UTIL_CAT(z_ztest_test_node_stats_, SUITE_NAME);                   \
	static const STRUCT_SECTION_ITERABLE(ztest_suite_node,                                     \
					     UTIL_CAT(z_ztest_test_node_, SUITE_NAME)) = {         \
		.name = STRINGIFY(SUITE_NAME),                                                     \
		.setup = (setup_fn),                                                               \
		.before = (before_fn),                                                             \
		.after = (after_fn),                                                               \
		.teardown = (teardown_fn),                                                         \
		.predicate = PREDICATE,                                                            \
		.stats = &UTIL_CAT(z_ztest_test_node_stats_, SUITE_NAME),                          \
	}
/**
 * Default entry point for running or listing registered unit tests.
 *
 * @param state The current state of the machine as it relates to the test executable.
 */
void ztest_run_all(const void *state);

/**
 * Run the registered unit tests which return true from their pragma function.
 *
 * @param state The current state of the machine as it relates to the test executable.
 * @return The number of tests that ran.
 */
__syscall int ztest_run_test_suites(const void *state);

#include <syscalls/ztest_test_new.h>

/**
 * @brief Fails the test if any of the registered tests did not run.
 *
 * When registering test suites, a pragma function can be provided to determine WHEN the test should
 * run. It is possible that a test suite could be registered but the pragma always prevents it from
 * running. In cases where a test should make sure that ALL suites ran at least once, this function
 * may be called at the end of test_main(). It will cause the test to fail if any suite was
 * registered but never ran.
 */
void ztest_verify_all_test_suites_ran(void);

/**
 * @brief Run a test suite.
 *
 * Internal implementation. Do not call directly. This will run the full test suite along with some
 * checks for fast failures and initialization.
 *
 * @param name The name of the suite to run.
 * @return Negative value if the test suite never ran; otherwise, return the number of failures.
 */
int z_ztest_run_test_suite(const char *name);

/**
 * @brief Returns next test within suite.
 *
 * @param suite Name of suite to get next test from.
 * @param prev  Previous unit test acquired from suite, use NULL to return first
 *		unit test.
 * @return struct ztest_unit_test*
 */
struct ztest_unit_test *z_ztest_get_next_test(const char *suite, struct ztest_unit_test *prev);

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
 * you can call this function from k_sys_fatal_error_handler to indicate that
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

#define Z_TEST(suite, fn, t_options, use_fixture)                                                  \
	static void _##suite##_##fn##_wrapper(void *data);                                         \
	static void suite##_##fn(                                                                  \
		COND_CODE_1(use_fixture, (struct suite##_fixture *fixture), (void)));              \
	static STRUCT_SECTION_ITERABLE(ztest_unit_test, z_ztest_unit_test_##suite##_##fn) = {      \
		.test_suite_name = STRINGIFY(suite),                                               \
		.name = STRINGIFY(fn),                                                             \
		.test = (_##suite##_##fn##_wrapper),                                               \
		.thread_options = t_options,                                                       \
	};                                                                                         \
	static void _##suite##_##fn##_wrapper(void *data)                                          \
	{                                                                                          \
		COND_CODE_1(use_fixture, (suite##_##fn((struct suite##_fixture *)data);),          \
			    (ARG_UNUSED(data); suite##_##fn();))                                   \
	}                                                                                          \
	static inline void suite##_##fn(                                                           \
		COND_CODE_1(use_fixture, (struct suite##_fixture *fixture), (void)))

#define Z_ZTEST(suite, fn, t_options) Z_TEST(suite, fn, t_options, 0)
#define Z_ZTEST_F(suite, fn, t_options) Z_TEST(suite, fn, t_options, 1)

/**
 * @brief Skips the test if config is enabled
 *
 * Use this macro at the start of your test case, to skip it when
 * config is enabled.  Useful when your test is still under development.
 *
 * @param config The Kconfig option used to skip the test.
 */
#define Z_TEST_SKIP_IFDEF(config) COND_CODE_1(config, (ztest_test_skip()), ())

/**
 * @brief Create and register a new unit test.
 *
 * Calling this macro will create a new unit test and attach it to the declared `suite`. The `suite`
 * does not need to be defined in the same compilation unit.
 *
 * @param suite The name of the test suite to attach this test
 * @param fn The test function to call.
 */
#define ZTEST(suite, fn) Z_ZTEST(suite, fn, 0)

/**
 * @brief Define a test function that should run as a user thread
 *
 * This macro behaves exactly the same as ZTEST, but calls the test function in user space if
 * `CONFIG_USERSPACE` was enabled.
 *
 * @param suite The name of the test suite to attach this test
 * @param fn The test function to call.
 */
#define ZTEST_USER(suite, fn) Z_ZTEST(suite, fn, K_USER)

/**
 * @brief Define a test function
 *
 * This macro behaves exactly the same as ZTEST(), but the function takes an argument for the
 * fixture of type `struct suite##_fixture*` named `this`.
 *
 * @param suite The name of the test suite to attach this test
 * @param fn The test function to call.
 */
#define ZTEST_F(suite, fn) Z_ZTEST_F(suite, fn, 0)

/**
 * @brief Define a test function that should run as a user thread
 *
 * If CONFIG_USERSPACE is not enabled, this is functionally identical to ZTEST_F(). The test
 * function takes a single fixture argument of type `struct suite##_fixture*` named `this`.
 *
 * @param suite The name of the test suite to attach this test
 * @param fn The test function to call.
 */
#define ZTEST_USER_F(suite, fn) Z_ZTEST_F(suite, fn, K_USER)

/**
 * @brief Test rule callback function signature
 *
 * The function signature that can be used to register a test rule's before/after callback. This
 * provides access to the test and the fixture data (if provided).
 *
 * @param test Pointer to the unit test in context
 * @param data Pointer to the test's fixture data (may be NULL)
 */
typedef void (*ztest_rule_cb)(const struct ztest_unit_test *test, void *data);

struct ztest_test_rule {
	ztest_rule_cb before_each;
	ztest_rule_cb after_each;
};

/**
 * @brief Define a test rule that will run before/after each unit test.
 *
 * Functions defined here will run before/after each unit test for every test suite. Along with the
 * callback, the test functions are provided a pointer to the test being run, and the data. This
 * provides a mechanism for tests to perform custom operations depending on the specific test or
 * the data (for example logging may use the test's name).
 *
 * Ordering:
 * - Test rule's `before` function will run before the suite's `before` function. This is done to
 * allow the test suite's customization to take precedence over the rule which is applied to all
 * suites.
 * - Test rule's `after` function is not guaranteed to run in any particular order.
 *
 * @param name The name for the test rule (must be unique within the compilation unit)
 * @param before_each_fn The callback function to call before each test (may be NULL)
 * @param after_each_fn The callback function to call after each test (may be NULL)
 */
#define ZTEST_RULE(name, before_each_fn, after_each_fn)                                            \
	static STRUCT_SECTION_ITERABLE(ztest_test_rule, z_ztest_test_rule_##name) = {              \
		.before_each = (before_each_fn),                                                   \
		.after_each = (after_each_fn),                                                     \
	}

extern struct ztest_test_rule _ztest_test_rule_list_start[];
extern struct ztest_test_rule _ztest_test_rule_list_end[];

/**
 * @brief A 'before' function to use in test suites that just need to start 1cpu
 *
 * Ignores data, and calls z_test_1cpu_start()
 *
 * @param data The test suite's data
 */
void ztest_simple_1cpu_before(void *data);

/**
 * @brief A 'after' function to use in test suites that just need to stop 1cpu
 *
 * Ignores data, and calls z_test_1cpu_stop()
 *
 * @param data The test suite's data
 */
void ztest_simple_1cpu_after(void *data);

/* definitions for use with testing application shared memory   */
#ifdef CONFIG_USERSPACE
#define ZTEST_DMEM K_APP_DMEM(ztest_mem_partition)
#define ZTEST_BMEM K_APP_BMEM(ztest_mem_partition)
#define ZTEST_SECTION K_APP_DMEM_SECTION(ztest_mem_partition)
extern struct k_mem_partition ztest_mem_partition;
#else
#define ZTEST_DMEM
#define ZTEST_BMEM
#define ZTEST_SECTION .data
#endif

/**
 * @brief Run the specified test suite.
 *
 * @param suite Test suite to run.
 */
#define ztest_run_test_suite(suite) z_ztest_run_test_suite(STRINGIFY(suite))

/**
 * @brief Structure for architecture specific APIs
 *
 */
struct ztest_arch_api {
	void (*run_all)(const void *state);
	bool (*should_suite_run)(const void *state, struct ztest_suite_node *suite);
	bool (*should_test_run)(const char *suite, const char *test);
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/ztest_test_new.h>

#endif /* ZEPHYR_TESTSUITE_ZTEST_TEST_H_ */
