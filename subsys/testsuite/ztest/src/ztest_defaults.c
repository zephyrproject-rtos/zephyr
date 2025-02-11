/*
 * Copyright (c) 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/llext/symbol.h>

/**
 * @brief Try to shorten a filename by removing the current directory
 *
 * This helps to reduce the very long filenames in assertion failures. It
 * removes the current directory from the filename and returns the rest.
 * This makes assertions a lot more readable, and sometimes they fit on one
 * line.
 *
 * @param file Filename to check
 * @returns Shortened filename, or @file if it could not be shortened
 */
const char *ztest_relative_filename(const char *file)
{
	return file;
}
EXPORT_SYMBOL(ztest_relative_filename);

/**
 * Default entry point for running registered unit tests.
 *
 * @param state The current state of the machine as it relates to the test executable.
 */
void z_ztest_run_all(const void *state, bool shuffle, int suite_iter, int case_iter)
{
	ztest_run_test_suites(state, shuffle, suite_iter, case_iter);
}

/**
 * @brief Determines if the test suite should run based on test cases listed
 *	  in the command line argument.
 *
 * @param state The current state of the machine as it relates to the test
 *		executable.
 * @param suite Pointer to ztest_suite_node
 * @return true
 * @return false
 */
bool z_ztest_should_suite_run(const void *state, struct ztest_suite_node *suite)
{
	bool run_suite = true;

	if (suite->predicate != NULL) {
		run_suite = suite->predicate(state);
	}

	return run_suite;
}

/**
 * @brief Determines if the test case should run based on test cases listed
 *	  in the command line argument. Run all tests for non-posix builds
 *
 * @param suite - name of test suite
 * @param test  - name of unit test
 * @return true
 * @return false
 */
bool z_ztest_should_test_run(const char *suite, const char *test)
{
	return true;
}

ZTEST_DMEM const struct ztest_arch_api ztest_api = {
	.run_all = z_ztest_run_all,
	.should_suite_run = z_ztest_should_suite_run,
	.should_test_run = z_ztest_should_test_run
};
