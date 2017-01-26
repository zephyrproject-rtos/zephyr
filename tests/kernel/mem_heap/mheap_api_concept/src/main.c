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
extern void test_mheap_malloc_free(void);
extern void test_mheap_malloc_align4(void);
extern void test_mheap_min_block_size(void);
extern void test_mheap_block_desc(void);

/*test case main entry*/
void test_main(void *p1, void *p2, void *p3)
{
	ztest_test_suite(test_mheap_api,
		ztest_unit_test(test_mheap_malloc_free),
		ztest_unit_test(test_mheap_malloc_align4),
		ztest_unit_test(test_mheap_min_block_size),
		ztest_unit_test(test_mheap_block_desc));
	ztest_run_test_suite(test_mheap_api);
}
