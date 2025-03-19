/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @defgroup kernel_message_queue_tests Message Queue
 * @ingroup all_tests
 * @{
 * @}
 */

#include <zephyr/ztest.h>

#include "test_msgq.h"

#ifdef CONFIG_64BIT
#define MAX_SZ	256
#else
#define MAX_SZ	128
#endif

K_HEAP_DEFINE(test_pool, MAX_SZ * 2);

extern struct k_msgq kmsgq;
extern struct k_msgq msgq;
extern struct k_sem end_sema;
extern struct k_thread tdata;
extern k_tid_t tids[2];
K_THREAD_STACK_DECLARE(tstack, STACK_SIZE);

void *msgq_api_setup(void)
{
	k_thread_access_grant(k_current_get(), &kmsgq, &msgq, &end_sema,
			      &tdata, &tstack);
	k_thread_heap_assign(k_current_get(), &test_pool);
	return NULL;
}

static void test_end_threads_join(void)
{
	for (int i = 0; i < ARRAY_SIZE(tids); i++) {
		if (tids[i] != NULL) {
			k_thread_join(tids[i], K_FOREVER);
			tids[i] = NULL;
		}
	}
}

static void msgq_api_test_after(void *data)
{
	test_end_threads_join();
}

static void msgq_api_test_1cpu_after(void *data)
{
	test_end_threads_join();

	ztest_simple_1cpu_after(data);
}

ZTEST_SUITE(msgq_api, NULL, msgq_api_setup, NULL, msgq_api_test_after, NULL);
ZTEST_SUITE(msgq_api_1cpu, NULL, msgq_api_setup,
	    ztest_simple_1cpu_before, msgq_api_test_1cpu_after, NULL);
