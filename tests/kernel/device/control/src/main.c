/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <init.h>
#include <ztest.h>
#include <sys/printk.h>

#include "fake_api.h"

static K_THREAD_STACK_DEFINE(test_stack, 512 + CONFIG_TEST_EXTRA_STACKSIZE);
static struct k_thread test_thread;
static K_SEM_DEFINE(concurrent_trigger, 0, 1);

enum test_triggers {
	TEST_TRIGGER_NONE = 0,
	TEST_TRIGGER_INT,
	TEST_TRIGGER_POLL,
	TEST_TRIGGER_TIMEOUT,
};

enum test_triggers trigger;

static struct device *dev;

static void concurrent_thread(void)
{
	while (1) {
		int ret;
		int value;

		k_sem_take(&concurrent_trigger, K_FOREVER);

		switch (trigger) {
		case TEST_TRIGGER_INT:
			ret = fake_lock_int_call(dev, &value);
			zassert_true((ret == 0), NULL);
			zassert_true((value == 1), NULL);
			break;
		case TEST_TRIGGER_POLL:
			ret = fake_lock_poll_call(dev, &value);
			zassert_true((ret == 0), NULL);
			zassert_true((value == 3), NULL);
			break;
		case TEST_TRIGGER_TIMEOUT:
			ret = fake_no_timeout_call(dev, &value);
			zassert_true((ret == 0), NULL);
			zassert_true((value == 5), NULL);
			break;
		default:
			break;
		}
	}
}

void test_init(void)
{
	trigger = TEST_TRIGGER_NONE;

	dev = device_get_binding(FAKE_DRV_NAME);
	zassert_false((dev == NULL), NULL);

	k_thread_create(&test_thread, test_stack,
			K_THREAD_STACK_SIZEOF(test_stack),
			(k_thread_entry_t)concurrent_thread,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);
}

void test_sync(void)
{
	int ret;

	ret = fake_sync_int_call(dev);

	zassert_true((ret == 0), NULL);

	ret = fake_sync_poll_call(dev);
	zassert_true((ret == 0), NULL);
}

#if defined(CONFIG_DEVICE_CONCURRENT_ACCESS)
void test_lock(void)
{
	int ret;
	int value;

	trigger = TEST_TRIGGER_INT;
	k_sem_give(&concurrent_trigger);
	k_yield();

	/* It will block until concurrent_thread got its result */
	ret = fake_lock_int_call(dev, &value);
	zassert_true((ret == 0), NULL);
	zassert_true((value == 2), NULL);

	trigger = TEST_TRIGGER_POLL;
	k_sem_give(&concurrent_trigger);
	k_yield();

	/* It will block until concurrent_thread got its result */
	ret = fake_lock_poll_call(dev, &value);
	zassert_true((ret == 0), NULL);
	zassert_true((value == 4), NULL);

#ifndef CONFIG_DEVICE_NO_LOCK_TIMEOUT
	trigger = TEST_TRIGGER_TIMEOUT;
	k_sem_give(&concurrent_trigger);
	k_yield();

	/* It must timeout as concurrent_thread is already calling */
	ret = fake_no_timeout_call(dev, &value);
	zassert_true((ret == -EAGAIN), NULL);
#endif
}
#else
void test_lock(void)
{
}
#endif /* CONFIG_DEVICE_CONCURRENT_ACCESS */

#if defined(CONFIG_DEVICE_STATUS_REPORT)
void test_status(void)
{
	int ret;

	ret = fake_status_test(dev, 2);
	zassert_equal(ret, 0, NULL);

	ret = fake_status_test(dev, 1);
	zassert_equal(ret, -EINVAL, NULL);

	ret = fake_status_test(dev, 2);
	zassert_equal(ret, 0, NULL);

	ret = fake_status_test(dev, INT_MAX);
	zassert_equal(ret, -EIO, NULL);

	ret = fake_status_test(dev, 2);
#ifdef CONFIG_DEVICE_STRICT_CHECK
	zassert_equal(ret, -EIO, NULL);
#else
	zassert_equal(ret, 0, NULL);
#endif
}
#else
void test_status(void)
{
}
#endif /* CONFIG_DEVICE_STATUS_REPORT */

void test_main(void)
{
	ztest_test_suite(device_control,
			 ztest_unit_test(test_init),
			 ztest_unit_test(test_sync),
			 ztest_unit_test(test_lock),
			 ztest_unit_test(test_status));

	ztest_run_test_suite(device_control);
}
