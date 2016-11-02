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

/*declare test cases */
extern void test_kdefine_preempt_thread(void);
extern void test_kdefine_coop_thread(void);
extern void test_kinit_preempt_thread(void);
extern void test_kinit_coop_thread(void);

/*test case main entry*/
void test_main(void *p1, void *p2, void *p3)
{
	ztest_test_suite(test_thread_init,
		ztest_unit_test(test_kdefine_preempt_thread),
		ztest_unit_test(test_kdefine_coop_thread),
		ztest_unit_test(test_kinit_preempt_thread),
		ztest_unit_test(test_kinit_coop_thread));
	ztest_run_test_suite(test_thread_init);
}
