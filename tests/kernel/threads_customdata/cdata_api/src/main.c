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
#include "test_cdata.h"

/**
 * @addtogroup t_threads_customdata
 * @{
 * @defgroup t_threads_customdata_api test_threads_customdata_api
 * @}
 */

/*test case main entry*/
void test_main(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	ztest_test_suite(test_customdata_api,
		ztest_unit_test(test_customdata_get_set_coop),
		ztest_unit_test(test_customdata_get_set_preempt));
	ztest_run_test_suite(test_customdata_api);
}
