/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
extern void test_mutex_lock_unlock(void);
extern void test_mutex_reent_lock_forever(void);
extern void test_mutex_reent_lock_no_wait(void);
extern void test_mutex_reent_lock_timeout_fail(void);
extern void test_mutex_reent_lock_timeout_pass(void);

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(mutex_api,
			 ztest_unit_test(test_mutex_lock_unlock),
			 ztest_unit_test(test_mutex_reent_lock_forever),
			 ztest_unit_test(test_mutex_reent_lock_no_wait),
			 ztest_unit_test(test_mutex_reent_lock_timeout_fail),
			 ztest_unit_test(test_mutex_reent_lock_timeout_pass)
			 );
	ztest_run_test_suite(mutex_api);
}
