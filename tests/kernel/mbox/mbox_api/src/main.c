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
extern void test_mbox_kinit(void);
extern void test_mbox_kdefine(void);
extern void test_mbox_put_get_null(void);
extern void test_mbox_put_get_buffer(void);
extern void test_mbox_async_put_get_buffer(void);
extern void test_mbox_async_put_get_block(void);
extern void test_mbox_target_source_thread_buffer(void);
extern void test_mbox_target_source_thread_block(void);

/*test case main entry*/
void test_main(void *p1, void *p2, void *p3)
{
	ztest_test_suite(test_mbox_api,
		ztest_unit_test(test_mbox_kinit),/*keep init first!*/
		ztest_unit_test(test_mbox_kdefine),
		ztest_unit_test(test_mbox_put_get_null),
		ztest_unit_test(test_mbox_put_get_buffer),
		ztest_unit_test(test_mbox_async_put_get_buffer),
		ztest_unit_test(test_mbox_async_put_get_block),
		ztest_unit_test(test_mbox_target_source_thread_buffer),
		ztest_unit_test(test_mbox_target_source_thread_block));
	ztest_run_test_suite(test_mbox_api);
}
