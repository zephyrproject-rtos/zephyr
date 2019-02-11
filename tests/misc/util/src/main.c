/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
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
#include <misc/util.h>

#define TEST_DEFINE_1 1
#define TEST_DEFINE_0 0

void test_COND_CODE_1(void)
{
	/* Test validates that expected code has been injected. Failure would
	 * be seen in compilation (lack of variable or ununsed variable.
	 */
	COND_CODE_1(1, (u32_t x0 = 1;), (u32_t y0;))
	zassert_true((x0 == 1), NULL);

	COND_CODE_1(NOT_EXISTING_DEFINE, (u32_t x1 = 1;), (u32_t y1 = 1;))
	zassert_true((y1 == 1), NULL);

	COND_CODE_1(TEST_DEFINE_1, (u32_t x2 = 1;), (u32_t y2 = 1;))
	zassert_true((x2 == 1), NULL);

	COND_CODE_1(2, (u32_t x3 = 1;), (u32_t y3 = 1;))
	zassert_true((y3 == 1), NULL);
}

void test_COND_CODE_0(void)
{
	/* Test validates that expected code has been injected. Failure would
	 * be seen in compilation (lack of variable or ununsed variable.
	 */
	COND_CODE_0(0, (u32_t x0 = 1;), (u32_t y0;))
	zassert_true((x0 == 1), NULL);

	COND_CODE_0(NOT_EXISTING_DEFINE, (u32_t x1 = 1;), (u32_t y1 = 1;))
	zassert_true((y1 == 1), NULL);

	COND_CODE_0(TEST_DEFINE_0, (u32_t x2 = 1;), (u32_t y2 = 1;))
	zassert_true((x2 == 1), NULL);

	COND_CODE_0(2, (u32_t x3 = 1;), (u32_t y3 = 1;))
	zassert_true((y3 == 1), NULL);
}

void test_UTIL_LISTIFY(void)
{
	int i = 0;

#define INC(x, _)		\
	do {			\
		i += x;		\
	} while (0);

#define DEFINE(x, _) int a##x;
#define MARK_UNUSED(x, _) ARG_UNUSED(a##x);

	UTIL_LISTIFY(4, DEFINE, _)
	UTIL_LISTIFY(4, MARK_UNUSED, _)

	UTIL_LISTIFY(4, INC, _)

	zassert_equal(i, 0 + 1 + 2 + 3, NULL);
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_util_api,
			 ztest_unit_test(test_COND_CODE_1),
			 ztest_unit_test(test_COND_CODE_0),
			 ztest_unit_test(test_UTIL_LISTIFY)
			 );
	ztest_run_test_suite(test_util_api);
}
