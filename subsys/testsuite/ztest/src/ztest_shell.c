/*
 * Copyright (c) 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/ztest.h>


static const char *test_args;

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

/**
 * Default entry point for running registered unit tests.
 *
 * @param state The current state of the machine as it relates to the test executable.
 */
void z_ztest_run_all(const void *state, bool shuffle, int suite_iter, int case_iter)
{
	ztest_run_test_suites(state, shuffle, suite_iter, case_iter);
}

void ztest_reset_test_args(void)
{
	if (test_args != NULL) {
		free((void *)test_args);
	}
	test_args = NULL;
}

void ztest_set_test_args(char *args)
{
	ztest_reset_test_args();
	test_args = strdup(args);
}

/**
 * @brief Helper function to get command line test arguments
 *
 * @return const char*
 */
char *ztest_get_test_args(void)
{
	if (test_args != NULL) {
		return strdup(test_args);
	} else {
		return NULL;
	}
}


/**
 * @brief Checks if the test_args contains the suite/test name.
 *
 * @param suite_name
 * @param test_name
 * @return true
 * @return false
 */
static bool z_ztest_testargs_contains(const char *suite_name, const char *test_name)
{
	bool found = false;
	char *test_args_local = ztest_get_test_args();
	char *suite_test_pair;
	char *last_suite_test_pair;
	char *suite_arg;
	char *test_arg;
	char *last_arg;

	if (test_args_local == NULL) {
		return true;
	}
	suite_test_pair = strtok_r(test_args_local, ",", &last_suite_test_pair);

	while (suite_test_pair && !found) {
		suite_arg = strtok_r(suite_test_pair, ":", &last_arg);
		test_arg = strtok_r(NULL, ":", &last_arg);

		found = !strcmp(suite_arg, suite_name);
		if (test_name && test_arg) {
			found &= !strcmp(test_arg, "*") ||
				 !strcmp(test_arg, test_name);
		}

		suite_test_pair = strtok_r(NULL, ",", &last_suite_test_pair);
	}
	free((void *)test_args_local);
	return found;
}

/**
 * @brief Determines if the test suite should run based on test cases listed
 *	  in the command line argument.
 *
 * Overrides implementation in ztest.c
 *
 * @param state The current state of the machine as it relates to the test
 *		executable.
 * @param suite Pointer to ztest_suite_node
 * @return true
 * @return false
 */
bool z_ztest_should_suite_run(const void *state, struct ztest_suite_node *suite)
{
	char *test_args_local = ztest_get_test_args();
	bool run_suite = true;

	if (test_args_local != NULL && !z_ztest_testargs_contains(suite->name, NULL)) {
		run_suite = false;
		suite->stats->run_count++;
	} else if (suite->predicate != NULL) {
		run_suite = suite->predicate(state);
	}

	if (test_args_local != NULL) {
		free((void *)test_args_local);
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
	bool run_test = false;

	run_test = z_ztest_testargs_contains(suite, test);

	return run_test;
}

ZTEST_DMEM const struct ztest_arch_api ztest_api = {
	.run_all = z_ztest_run_all,
	.should_suite_run = z_ztest_should_suite_run,
	.should_test_run = z_ztest_should_test_run
};
