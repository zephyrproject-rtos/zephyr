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
 * @addtogroup t_pipe
 * @{
 * @defgroup t_pipe_api test_pipe_api
 * @}
 */

#include <ztest.h>
extern void test_pipe_thread2thread(void);
extern void test_pipe_thread2isr(void);
extern void test_pipe_put_fail(void);
extern void test_pipe_get_fail(void);

/*test case main entry*/
void test_main(void *p1, void *p2, void *p3)
{
	ztest_test_suite(test_pipe_api,
		ztest_unit_test(test_pipe_thread2thread),
		ztest_unit_test(test_pipe_thread2isr),
		ztest_unit_test(test_pipe_put_fail),
		ztest_unit_test(test_pipe_get_fail));
	ztest_run_test_suite(test_pipe_api);
}
