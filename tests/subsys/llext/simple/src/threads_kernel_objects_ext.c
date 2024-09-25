/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This code demonstrates the use of threads and requires object
 * relocation support.
 */

#include <stdint.h>
#include <zephyr/llext/symbol.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest_assert.h>

#include "threads_kernel_objects_ext.h"

void test_thread(void *arg0, void *arg1, void *arg2)
{
	printk("Take semaphore from test thread\n");
	k_sem_take(&my_sem, K_FOREVER);
}

void test_entry(void)
{
	printk("Give semaphore from main thread\n");
	zassert_not_null(&my_sem);
	k_sem_give(&my_sem);

	printk("Creating thread\n");
	zassert_not_null(&my_thread);
	zassert_not_null(&my_thread_stack);
	k_tid_t tid = k_thread_create(&my_thread, (k_thread_stack_t *) &my_thread_stack,
				MY_THREAD_STACK_SIZE, &test_thread, NULL, NULL, NULL,
				MY_THREAD_PRIO, MY_THREAD_OPTIONS, K_FOREVER);

	printk("Starting thread\n");
	k_thread_start(tid);

	printk("Joining thread\n");
	k_thread_join(&my_thread, K_FOREVER);
	printk("Test thread joined\n");
}
LL_EXTENSION_SYMBOL(test_entry);
