/*
 * Copyright (c) 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include "cmdline.h" /* native_sim command line options header */
#include "soc.h"
#include <zephyr/tc_util.h>
#include <zephyr/ztest_test.h>
#include "nsi_host_trampolines.h"

static const char *test_args;
static bool list_tests;

static void add_test_filter_option(void)
{
	static struct args_struct_t test_filter_s[] = {
		/*
		 * Fields:
		 * manual, mandatory, switch,
		 * option_name, var_name ,type,
		 * destination, callback,
		 * description
		 */
		{ false, false, true, "list", NULL, 'b', (void *)&list_tests, NULL,
		  "List all suite and test cases" },
		{ false, false, false, "test", "suite::test", 's', (void *)&test_args, NULL,
		  "Name of tests to run. Comma separated list formatted as "
		  "\'suiteA::test1,suiteA::test2,suiteB::*\'. An * can be used "
		  "as a wildcard to run all tests within a suite." },
		ARG_TABLE_ENDMARKER
	};

	native_add_command_line_opts(test_filter_s);
}

NATIVE_TASK(add_test_filter_option, PRE_BOOT_1, 10);

/**
 * @brief Try to shorten a filename by removing the current directory
 *
 * This helps to reduce the very long filenames in assertion failures. It
 * removes the current directory from the filename and returns the rest.
 * This makes assertions a lot more readable, and sometimes they fit on one
 * line.
 *
 * Overrides implementation in ztest.c
 *
 * @param file Filename to check
 * @returns Shortened filename, or @file if it could not be shortened
 */
const char *ztest_relative_filename(const char *file)
{
	const char *cwd;
	char buf[200];

	cwd = nsi_host_getcwd(buf, sizeof(buf));
	if (cwd && strlen(file) > strlen(cwd) && !strncmp(file, cwd, strlen(cwd))) {
		return file + strlen(cwd) + 1; /* move past the trailing '/' */
	}
	return file;
}

/**
 * @brief Helper function to set list_tests
 *
 * @param value - Sets list_tests to value
 */
void ztest_set_list_test(bool value)
{
	list_tests = value;
}

/**
 * @brief Helper function to get command line argument for listing tests
 *
 * @return true
 * @return false
 */
bool z_ztest_get_list_test(void)
{
	return list_tests;
}

/**
 * @brief Helper function to get command line test arguments
 *
 * @return const char*
 */
const char *ztest_get_test_args(void)
{
	return test_args;
}



/**
 * @brief Lists registered unit tests in this binary, one per line
 *
 * @return int Number of tests in binary
 */
int z_ztest_list_tests(void)
{
	struct ztest_suite_node *ptr;
	struct ztest_unit_test *test = NULL;
	int test_count = 0;
	static bool list_once = true;

	if (list_once) {
		for (ptr = _ztest_suite_node_list_start; ptr < _ztest_suite_node_list_end; ++ptr) {
			test = NULL;
			while ((test = z_ztest_get_next_test(ptr->name, test)) != NULL) {
				TC_PRINT("%s::%s\n", test->test_suite_name, test->name);
				test_count++;
			}
		}
		list_once = false;
	}

	return test_count;
}

/**
 * Default entry point for running or listing registered unit tests.
 *
 * @param state The current state of the machine as it relates to the test executable.
 */
void z_ztest_run_all(const void *state, bool shuffle, int suite_iter, int case_iter)
{
	if (z_ztest_get_list_test()) {
		z_ztest_list_tests();
	} else {
		ztest_run_test_suites(state, shuffle, suite_iter, case_iter);
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
	char *test_args_local = nsi_host_strdup(test_args);
	char *suite_test_pair;
	char *last_suite_test_pair;
	char *suite_arg;
	char *test_arg;
	char *last_arg;

	suite_test_pair = strtok_r(test_args_local, ",", &last_suite_test_pair);

	while (suite_test_pair && !found) {
		suite_arg = strtok_r(suite_test_pair, ":", &last_arg);
		test_arg = strtok_r(NULL, ":", &last_arg);

		found = !strcmp(suite_arg, suite_name);
		if (test_name) {
			found &= !strcmp(test_arg, "*") ||
				 !strcmp(test_arg, test_name);
		}

		suite_test_pair = strtok_r(NULL, ",", &last_suite_test_pair);
	}

	nsi_host_free(test_args_local);
	return found;
}

/**
 * @brief Determines if the test case should run based on test cases listed
 *	  in the command line argument.
 *
 * Overrides implementation in ztest.c
 *
 * @param suite - name of test suite
 * @param test  - name of unit test
 * @return true
 * @return false
 */
bool z_ztest_should_test_run(const char *suite, const char *test)
{
	bool run_test = false;

	run_test = (test_args == NULL ||
		    z_ztest_testargs_contains(suite, test));

	return run_test;
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
	bool run_suite = true;

	if (test_args != NULL && !z_ztest_testargs_contains(suite->name, NULL)) {
		run_suite = false;
		suite->stats->run_count++;
	} else if (suite->predicate != NULL) {
		run_suite = suite->predicate(state);
	}

	return run_suite;
}

ZTEST_DMEM const struct ztest_arch_api ztest_api = {
	.run_all = z_ztest_run_all,
	.should_suite_run = z_ztest_should_suite_run,
	.should_test_run = z_ztest_should_test_run
};
