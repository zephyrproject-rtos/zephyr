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

__syscall void z_test_1cpu_start(void);
__syscall void z_test_1cpu_stop(void);

/**
 * @brief Define a SMP-unsafe test function
 *
 * As ztest_unit_test(), but ensures all test code runs on only
 * one CPU when in SMP.
 *
 * @param fn Test function
 */
#ifdef CONFIG_SMP
#define ztest_1cpu_unit_test(fn)					\
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
#define ztest_1cpu_user_unit_test(fn) \
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

/**
 * @}
 */
#ifndef ZTEST_UNITTEST
#include <syscalls/ztest_test.h>
#endif

#ifdef __cplusplus
}
#endif

#endif /* __ZTEST_ASSERT_H__ */
