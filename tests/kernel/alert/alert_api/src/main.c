/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
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
