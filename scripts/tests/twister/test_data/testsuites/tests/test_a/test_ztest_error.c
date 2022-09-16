/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

static void test_1b(void)
{
	ztest_test_skip();
}

void test_main(void)
{
	#ifdef TEST_feature1
		ztest_test_suite(feature1,
		ztest_unit_test(1a), ztest_unit_test(test_1b),
		ztest_unit_test(test_1c)
	);
	#endif
	#ifdef TEST_feature2
		ztest_test_suite(feature2,
		ztest_unit_test(test_2a),
		ztest_unit_test(test_2b)
		);
		ztest_run_test_suite(feature2);
	#endif
}
