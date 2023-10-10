/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/ztest.h>

#define STACKSIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

int function_add(int val, int incr)
{
	return val + incr;
}

int function_1_1(int val)
{
	int res = val;

	for (int i = 0; i < val; i++) {
		res = function_add(res, 1);
	}

	TC_PRINT("TEST_GDB_CMD:Exit:%s res=%d\n", __func__, res);
	return res;
}

int function_1(int val)
{
	int res = 0;
	int mult = 2;

	res += val * mult;
	res = function_1_1(res);

	TC_PRINT("TEST_GDB_CMD:Exit:%s res=%d\n", __func__, res);
	return res;
}

/**
 * @brief Test GDB stub connection and basic breakpoint commands.
 *
 */
ZTEST(gdbstub, test_gdb_breakpoints_basic)
{
	TC_PRINT("TEST_GDB_CMD:Start\n");
	/* Check expected result without GDB interference. */
	zassert_equal(40, function_1(10), "function_1() failed");
	/* Expect value changes from GDB while executing. */
	zassert_equal((40 + 20), function_1(10),
		      "FAILED function_1() expecting GDB adds 10*2 to the result");
	TC_PRINT("TEST_GDB_CMD:Done\n");
}

ZTEST_SUITE(gdbstub, NULL, NULL, NULL, NULL, NULL);
