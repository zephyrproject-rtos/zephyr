/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
extern void test_boot_page_table(void);

void test_main(void)
{
	ztest_test_suite(boot_page_table_validate,
			 ztest_unit_test(test_boot_page_table));
	ztest_run_test_suite(boot_page_table_validate);
}
