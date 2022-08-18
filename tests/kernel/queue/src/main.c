/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include "test_queue.h"

#ifdef CONFIG_64BIT
#define MAX_SZ	128
#else
#define MAX_SZ	96
#endif
K_HEAP_DEFINE(test_pool, MAX_SZ * 4 + 128);

/*test case main entry*/
void *queue_test_setup(void)
{
	k_thread_heap_assign(k_current_get(), &test_pool);

	return NULL;
}

ZTEST_SUITE(queue_api, NULL, queue_test_setup, NULL, NULL, NULL);
ZTEST_SUITE(queue_api_1cpu, NULL, queue_test_setup,
		ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
