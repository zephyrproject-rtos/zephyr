/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "soc.h"
#include "kernel.h"
#include "posix_board_if.h"

/**
 * @brief Test NATIVE_TASK hook for native builds
 *
 * Verify that the NATIVE_TASK hooks are registered and called.
 * Note that the ztest framework cannot be used as we are testing
 * functionality which executes before and after all Zephyr threads have been
 * terminated.
 */
static void test_check(int hook)
{
	static int call_nbr;
	static int failed;
	static int expected_order[8] = {1, 2, 3, 8, 6, 5, 4, 7};

	if (failed) {
		return;
	}

	posix_print_trace("test_hook%i called\n", hook);

	if (expected_order[call_nbr] != hook) {
		failed = 1;
		posix_print_trace("PROJECT EXECUTION FAILED\n");
	}

	if (call_nbr++ == 7) {
		posix_print_trace("PROJECT EXECUTION SUCCESSFUL\n");
	}
}

#define TEST_HOOK(n)			\
	static void test_hook##n(void)	\
	{				\
		test_check(n);	\
	}

TEST_HOOK(1);
TEST_HOOK(2);
TEST_HOOK(3);
TEST_HOOK(4);
TEST_HOOK(5);
TEST_HOOK(6);
TEST_HOOK(7);
TEST_HOOK(8);

NATIVE_TASK(test_hook1, PRE_BOOT_1, 1);
NATIVE_TASK(test_hook2, PRE_BOOT_2, 200);
NATIVE_TASK(test_hook3, PRE_BOOT_3, 20);
NATIVE_TASK(test_hook8, FIRST_SLEEP, 5);
NATIVE_TASK(test_hook4, ON_EXIT, 200);
NATIVE_TASK(test_hook5, ON_EXIT, 20);
NATIVE_TASK(test_hook6, ON_EXIT, 1);
NATIVE_TASK(test_hook7, ON_EXIT, 310);

void main(void)
{
	k_sleep(100);
	posix_exit(0);
}
