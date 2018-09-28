/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include "test_queue.h"

#ifndef CONFIG_USERSPACE
static void test_queue_supv_to_user(void)
{
	ztest_test_skip();
}

static void test_auto_free(void)
{
	ztest_test_skip();
}
#endif

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(queue_api,
			 ztest_unit_test(test_queue_supv_to_user),
			 ztest_unit_test(test_auto_free),
			 ztest_unit_test(test_queue_thread2thread),
			 ztest_unit_test(test_queue_thread2isr),
			 ztest_unit_test(test_queue_isr2thread),
			 ztest_unit_test(test_queue_get_2threads),
			 ztest_unit_test(test_queue_get_fail),
			 ztest_unit_test(test_queue_loop),
			 ztest_unit_test(test_queue_alloc));
	ztest_run_test_suite(queue_api);
}
