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

/**
 * @addtogroup t_driver_wdt
 * @{
 * @defgroup t_wdt_basic test_wdt_basic_operations
 * @}
 */

#include <zephyr.h>
#include <ztest.h>

#ifdef INT_RESET
extern void test_wdt_int_reset_26(void);
#else
extern void test_wdt_reset_26(void);
#endif

void test_main(void)
{
	ztest_test_suite(wdt_basic_test,
#ifdef INT_RESET
			 ztest_unit_test(test_wdt_int_reset_26));
#else
			 ztest_unit_test(test_wdt_reset_26));
#endif
	ztest_run_test_suite(wdt_basic_test);
}
