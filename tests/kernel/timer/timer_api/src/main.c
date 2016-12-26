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
 * @addtogroup t_kernel_timer
 * @{
 * @defgroup t_timer_api test_timer_api
 * @brief TestPurpose: verify timer api functionality
 * @}
 */

#include "test_timer.h"
#include <ztest.h>

/*test case main entry*/
void test_main(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	ztest_test_suite(test_timer_api,
		ztest_unit_test(test_timer_duration_period),
		ztest_unit_test(test_timer_period_0),
		ztest_unit_test(test_timer_expirefn_null),
		ztest_unit_test(test_timer_status_get),
		ztest_unit_test(test_timer_status_get_anytime),
		ztest_unit_test(test_timer_status_sync),
		ztest_unit_test(test_timer_k_define));
	ztest_run_test_suite(test_timer_api);
}
