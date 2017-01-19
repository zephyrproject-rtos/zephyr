/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_sema
 * @{
 * @defgroup t_sema_api test_sema_api
 * @}
 */


#include <ztest.h>
extern void test_sema_thread2thread(void);
extern void test_sema_thread2isr(void);
extern void test_sema_reset(void);
extern void test_sema_count_get(void);

/*test case main entry*/
void test_main(void *p1, void *p2, void *p3)
{
	ztest_test_suite(test_sema_api,
			 ztest_unit_test(test_sema_thread2thread),
			 ztest_unit_test(test_sema_thread2isr),
			 ztest_unit_test(test_sema_reset),
			 ztest_unit_test(test_sema_count_get));
	ztest_run_test_suite(test_sema_api);
}
