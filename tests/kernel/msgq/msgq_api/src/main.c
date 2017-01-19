/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_kernel_msgq
 * @{
 * @defgroup t_msgq_api test_msgq_api
 * @}
 */

#include <ztest.h>
extern void test_msgq_thread(void);
extern void test_msgq_isr(void);
extern void test_msgq_put_fail(void);
extern void test_msgq_get_fail(void);
extern void test_msgq_purge_when_put(void);

/*test case main entry*/
void test_main(void *p1, void *p2, void *p3)
{
	ztest_test_suite(test_msgq_api,
			 ztest_unit_test(test_msgq_thread),
			 ztest_unit_test(test_msgq_isr),
			 ztest_unit_test(test_msgq_put_fail),
			 ztest_unit_test(test_msgq_get_fail),
			 ztest_unit_test(test_msgq_purge_when_put));
	ztest_run_test_suite(test_msgq_api);
}
