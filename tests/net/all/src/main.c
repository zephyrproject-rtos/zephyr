/* main.c - Application main entry point */

/* We are just testing that this program compiles ok with all possible
 * network related Kconfig options enabled.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

static void ok(void)
{
	zassert_true(true, "This test should never fail");
}

void test_main(void)
{
	ztest_test_suite(net_compile_all_test,
			 ztest_unit_test(ok)
			 );

	ztest_run_test_suite(net_compile_all_test);
}
