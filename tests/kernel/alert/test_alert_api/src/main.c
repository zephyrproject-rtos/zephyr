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
 * @addtogroup t_kernel_alert
 * @{
 * @defgroup t_alert_api test_alert_api
 * @}
 */

#include <ztest.h>
extern void test_thread_alert_default(void);
extern void test_thread_alert_ignore(void);
extern void test_thread_alert_consumed(void);
extern void test_thread_alert_pending(void);
extern void test_isr_alert_default(void);
extern void test_isr_alert_ignore(void);
extern void test_isr_alert_consumed(void);
extern void test_isr_alert_pending(void);
extern void test_thread_kinit_alert(void);
extern void test_isr_kinit_alert(void);

/*test case main entry*/
void test_main(void *p1, void *p2, void *p3)
{
	ztest_test_suite(test_alert_api,
		ztest_unit_test(test_thread_alert_default),
		ztest_unit_test(test_thread_alert_ignore),
		ztest_unit_test(test_thread_alert_consumed),
		ztest_unit_test(test_thread_alert_pending),
		ztest_unit_test(test_isr_alert_default),
		ztest_unit_test(test_isr_alert_ignore),
		ztest_unit_test(test_isr_alert_consumed),
		ztest_unit_test(test_isr_alert_pending),
		ztest_unit_test(test_thread_kinit_alert),
		ztest_unit_test(test_isr_kinit_alert));
	ztest_run_test_suite(test_alert_api);
}
