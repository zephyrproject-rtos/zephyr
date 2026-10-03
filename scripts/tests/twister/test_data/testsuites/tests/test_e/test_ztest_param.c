/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

ZTEST_SUITE(feature7, NULL, NULL, NULL, NULL, NULL);

ZTEST(feature7, test_plain);

ZTEST_P(feature7, test_param);
ZTEST_DEFINE_PARAM_VALUES(param_vals, int, 1, 2, 3);
ZTEST_INSTANTIATE_TEST_SUITE_P(vals, feature7, test_param, param_vals);

ZTEST_USER_P(feature7, test_user_param);
ZTEST_INSTANTIATE_TEST_SUITE_P(vals, feature7, test_user_param, param_vals);

ztest_run_registered_test_suites(feature7);
