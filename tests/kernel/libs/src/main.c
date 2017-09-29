/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

extern void limits_test(void);
extern void stdbool_test(void);
extern void stddef_test(void);
extern void stdint_test(void);
extern void string_test(void);

void test_main(void)
{
	ztest_test_suite(test_libs,
			ztest_unit_test(limits_test),
			ztest_unit_test(stdbool_test),
			ztest_unit_test(stddef_test),
			ztest_unit_test(stdint_test),
			ztest_unit_test(string_test));
	ztest_run_test_suite(test_libs);
}
