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
 * @addtogroup t_kernel_threads
 * @{
 * @defgroup t_threads_scheduling test_threads_scheduling
 * @brief TestPurpose: verify threads scheduling based on priority, time
 * slice and lock/unlock
 * @}
 */

#include "test_sched.h"

/*test case main entry*/
void test_main(void *p1, void *p2, void *p3)
{
	ztest_test_suite(test_threads_scheduling,
		ztest_unit_test(test_priority_cooperative),
		ztest_unit_test(test_priority_preemptible),
		ztest_unit_test(test_yield_cooperative),
		ztest_unit_test(test_sleep_cooperative),
		ztest_unit_test(test_sleep_wakeup_preemptible),
		ztest_unit_test(test_time_slicing_preemptible),
		ztest_unit_test(test_time_slicing_disable_preemptible),
		ztest_unit_test(test_lock_preemptible),
		ztest_unit_test(test_unlock_preemptible)
		);
	ztest_run_test_suite(test_threads_scheduling);
}
