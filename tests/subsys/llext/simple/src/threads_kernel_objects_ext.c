/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This code checks the functionality of threads, synchronization primitives
 * and device access from extensions.
 * This test should be valid from both user and privileged modes.
 */

#include <stdint.h>
#include <zephyr/llext/symbol.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest_assert.h>

#include "threads_kernel_objects_ext.h"

/*
 * Some platforms do not define any usable DT devices (not even the console).
 * In those cases the device API test can't be executed.
 */
#if DT_HAS_CHOSEN(zephyr_console) && DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(zephyr_console))
#define CONSOLE_DT_NODE DT_CHOSEN(zephyr_console)
#endif

void test_thread(void *arg0, void *arg1, void *arg2)
{
	printk("Take semaphore from test thread\n");
	k_sem_take(&my_sem, K_FOREVER);

#ifdef CONSOLE_DT_NODE
	const struct device *const console_dev = DEVICE_DT_GET(CONSOLE_DT_NODE);
	const char *const console_name = DEVICE_DT_NAME(CONSOLE_DT_NODE);
	const struct device *binding_dev;

	/* Ensure the console device was properly obtained at compile time */
	zassert_not_null(console_dev);

	/* Try to get the same handle at runtime and verify they match */
	binding_dev = device_get_binding(console_name);
	zassert_equal(binding_dev, console_dev);

	/* Verify device API functionality, console must be ready in CI tests */
	zassert_true(device_is_ready(console_dev));
#endif
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
EXPORT_SYMBOL(test_entry);
