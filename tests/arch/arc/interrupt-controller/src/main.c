/*
 * Copyright (c) 2021 Synopsys, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

extern void test_initial_state(void);

void test_main(void)
{
	ztest_test_suite(interrupt_controller,
		ztest_unit_test(test_initial_state));
	ztest_run_test_suite(interrupt_controller);
}
