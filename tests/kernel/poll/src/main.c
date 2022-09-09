/*
 * Copyright (c) 2017 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#ifdef CONFIG_64BIT
#define MAX_SZ	256
#else
#define MAX_SZ	128
#endif

K_HEAP_DEFINE(test_heap, MAX_SZ * 4);
extern void poll_test_grant_access(void);
extern void poll_fail_grant_access(void);

/*test case main entry*/
static void *poll_setup(void)
{
	poll_test_grant_access();
	poll_fail_grant_access();

	k_thread_heap_assign(k_current_get(), &test_heap);

	return NULL;
}

ZTEST_SUITE(poll_api, NULL, poll_setup, NULL, NULL, NULL);
ZTEST_SUITE(poll_api_1cpu, NULL, poll_setup, ztest_simple_1cpu_before,
		 ztest_simple_1cpu_after, NULL);
