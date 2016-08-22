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

static void empty_test(void)
{
}

static void assert_tests(void)
{
	assert_true(1, NULL);
	assert_false(0, NULL);
	assert_is_null(NULL, NULL);
	assert_not_null("foo", NULL);
	assert_equal(1, 1, NULL);
	assert_equal_ptr(NULL, NULL, NULL);
}

void test_main(void)
{
	ztest_test_suite(framework_tests,
			 ztest_unit_test(empty_test),
			 ztest_unit_test(assert_tests)
			 );

	ztest_run_test_suite(framework_tests);
}
