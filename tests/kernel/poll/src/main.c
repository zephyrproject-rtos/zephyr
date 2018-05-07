/*
 * Copyright (c) 2017 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_poll
 * @{
 * @defgroup t_poll_api test_poll_api
 * @}
 */

#include <ztest.h>
extern void test_poll_no_wait(void);
extern void test_poll_wait(void);
extern void test_poll_multi(void);
extern void test_poll_grant_access(void);

K_MEM_POOL_DEFINE(test_pool, 128, 128, 4, 4);

/*test case main entry*/
void test_main(void)
{
	test_poll_grant_access();

	k_thread_resource_pool_assign(k_current_get(), &test_pool);

	ztest_test_suite(poll_api,
			ztest_user_unit_test(test_poll_no_wait),
			ztest_unit_test(test_poll_wait),
			ztest_unit_test(test_poll_multi));
	ztest_run_test_suite(poll_api);
}
