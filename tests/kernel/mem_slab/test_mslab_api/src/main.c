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

extern void test_mslab_kinit(void);
extern void test_mslab_kdefine(void);
extern void test_mslab_kdefine_extern(void);
extern void test_mslab_alloc_free_thread(void);
extern void test_mslab_alloc_free_isr(void);
extern void test_mslab_alloc_align(void);
extern void test_mslab_alloc_timeout(void);
extern void test_mslab_used_get(void);

/*test case main entry*/
void test_main(void *p1, void *p2, void *p3)
{
	ztest_test_suite(test_mslab_api,
		ztest_unit_test(test_mslab_kinit),
		ztest_unit_test(test_mslab_kdefine),
		ztest_unit_test(test_mslab_kdefine_extern),
		ztest_unit_test(test_mslab_alloc_free_thread),
		ztest_unit_test(test_mslab_alloc_free_isr),
		ztest_unit_test(test_mslab_alloc_align),
		ztest_unit_test(test_mslab_alloc_timeout),
		ztest_unit_test(test_mslab_used_get));
	ztest_run_test_suite(test_mslab_api);
}
