/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
extern void test_key_chain(void);
extern void test_vectors(void);
extern void test_fixed_key_variable_text(void);
extern void test_variable_key_fixed_text(void);

/**test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_aes_fn,
		ztest_unit_test(test_key_chain),
		ztest_unit_test(test_vectors),
		ztest_unit_test(test_fixed_key_variable_text),
		ztest_unit_test(test_variable_key_fixed_text));
	ztest_run_test_suite(test_aes_fn);
}
