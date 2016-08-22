/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ztest.h>

static void expect_one_parameter(int a)
{
	ztest_check_expected_value(a);
}

static void expect_two_parameters(int a, int b)
{
	ztest_check_expected_value(a);
	ztest_check_expected_value(b);
}

static void parameter_tests(void)
{
	ztest_expect_value(expect_one_parameter, a, 1);
	expect_one_parameter(1);

	ztest_expect_value(expect_two_parameters, a, 2);
	ztest_expect_value(expect_two_parameters, b, 3);
	expect_two_parameters(2, 3);
}

static int returns_int(void)
{
	return ztest_get_return_value();
}

static void return_value_tests(void)
{
	ztest_returns_value(returns_int, 5);
	assert_equal(returns_int(), 5, NULL);
}

void test_main(void)
{
	ztest_test_suite(mock_framework_tests,
			 ztest_unit_test(parameter_tests),
			 ztest_unit_test(return_value_tests)
			 );

	ztest_run_test_suite(mock_framework_tests);
}
