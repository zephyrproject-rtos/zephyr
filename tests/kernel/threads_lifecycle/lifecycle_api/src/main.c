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
 * @addtogroup t_threads_lifecycle
 * @{
 * @defgroup t_threads_lifecycle_api test_threads_lifecycle_api
 * @brief TestPurpose: verify zephyr basic threads lifecycle apis
 * @}
 */

#include <ztest.h>
extern void test_threads_spawn_params(void);
extern void test_threads_spawn_priority(void);
extern void test_threads_spawn_delay(void);
extern void test_threads_suspend_resume_cooperative(void);
extern void test_threads_suspend_resume_preemptible(void);
extern void test_threads_cancel_undelayed(void);
extern void test_threads_cancel_delayed(void);
extern void test_threads_cancel_started(void);
extern void test_threads_abort_self(void);
extern void test_threads_abort_others(void);

/*test case main entry*/
void test_main(void *p1, void *p2, void *p3)
{
	ztest_test_suite(test_threads_lifecycle,
			 ztest_unit_test(test_threads_spawn_params),
			 ztest_unit_test(test_threads_spawn_priority),
			 ztest_unit_test(test_threads_spawn_delay),
			 ztest_unit_test(test_threads_suspend_resume_cooperative),
			 ztest_unit_test(test_threads_suspend_resume_preemptible),
			 ztest_unit_test(test_threads_cancel_undelayed),
			 ztest_unit_test(test_threads_cancel_delayed),
			 ztest_unit_test(test_threads_cancel_started),
			 ztest_unit_test(test_threads_abort_self),
			 ztest_unit_test(test_threads_abort_others)
			 );
	ztest_run_test_suite(test_threads_lifecycle);
}
