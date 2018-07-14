/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "soc.h"

/**
 * @brief Test NATIVE_EXIT_TASK hook for native builds
 *
 * Verify that the NATIVE_EXIT_TASK hooks are registered and called on exit.
 * Note that the ztest framework cannot be used as we are testing
 * functionality which executes after all Zephyr threads have been terminated.
 */
static void test_exit_hook(void)
{
	static int hooks_count;

	hooks_count++;
	posix_print_trace("%s called %i of 5 times\n", __func__, hooks_count);
	if (hooks_count == 5) {
		posix_print_trace("PROJECT EXECUTION SUCCESSFUL\n");
	}
}

static void test_exit_hook1(void)
{
	test_exit_hook();
}

static void test_exit_hook2(void)
{
	test_exit_hook();
}

static void test_exit_hook3(void)
{
	test_exit_hook();
}

static void test_exit_hook4(void)
{
	test_exit_hook();
}

static void test_exit_hook5(void)
{
	test_exit_hook();
}

NATIVE_EXIT_TASK(test_exit_hook1);
NATIVE_EXIT_TASK(test_exit_hook2);
NATIVE_EXIT_TASK(test_exit_hook3);
NATIVE_EXIT_TASK(test_exit_hook4);
NATIVE_EXIT_TASK(test_exit_hook5);

void main(void)
{
	posix_exit(0);
}
