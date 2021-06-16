/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <kernel.h>
#include <pm/pm.h>
#include <pm/device_runtime.h>
#include "dummy_driver.h"

#define MAX_TIMES 10
#define STACKSIZE 1024

/* Semaphore used to synchronize thread A and thread B*/
K_SEM_DEFINE(sem, 0, 1);

K_THREAD_STACK_DEFINE(threadA_stack, STACKSIZE);
K_THREAD_STACK_DEFINE(threadB_stack, STACKSIZE);

static const struct device *dev;
static struct dummy_driver_api *api;

static struct k_thread threadA;
static struct k_thread threadB;


void threadA_func(void *arg1, void *arg2, void *arg3)
{
	int ret;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	ret = api->open(dev);
	zassert_true(ret == 0, "Fail to get device");

	/* Lets allow threadB run */
	k_sem_give(&sem);

	/* Block waiting for device operation conclude */
	ret = api->wait(dev);
	zassert_true(ret == 0, "Fail to wait transaction");

	/* At this point threadB should have put the device and
	 * the current state should be SUSPENDED.
	 */
	zassert_true(dev->pm->state == PM_DEVICE_STATE_SUSPEND, "Wrong state");

	k_sem_take(&sem, K_FOREVER);

	ret = api->open(dev);
	zassert_true(ret == 0, "Fail to get device");

	/* Lets allow threadB run */
	k_sem_give(&sem);

	ret = api->wait(dev);
	zassert_true(ret == 0, "Fail to wait transaction");

	zassert_true(dev->pm->state == PM_DEVICE_STATE_ACTIVE, "Wrong state");
}

void threadB_func(void *arg1, void *arg2, void *arg3)
{
	int ret;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	k_sem_take(&sem, K_FOREVER);

	api->close(dev);

	k_sem_give(&sem);
	ret = api->wait(dev);
	zassert_true(ret == 0, "Fail to wait transaction");

	/* Check the state */
	zassert_true(dev->pm->state == PM_DEVICE_STATE_SUSPEND, "Wrong state");
}

/*
 * @brief test device runtime concurrency
 *
 * @details
 *  - Two threads will do different operations on a device. ThreadA will
 *    try to bring up the device using an async call and then will be scheduled
 *    out and threadB will run. ThreadB then will suspend the device and give up
 *    in favor of threadA. At this point the device should reflect these
 *    operations and be suspended.
 *
 * @see pm_device_get_async(), pm_device_put_async()
 *
 * @ingroup power_tests
 */
void test_concurrency(void)
{
	k_thread_start(&threadA);
	k_thread_start(&threadB);

	k_thread_join(&threadA, K_FOREVER);
	k_thread_join(&threadB, K_FOREVER);
}

void test_setup(void)
{
	dev = device_get_binding(DUMMY_DRIVER_NAME);
	api = (struct dummy_driver_api *)dev->api;

	k_thread_create(&threadA, threadA_stack,
			K_THREAD_STACK_SIZEOF(threadA_stack),
			threadA_func, NULL, NULL, NULL,
			K_PRIO_PREEMPT(1), 0, K_FOREVER);

	/* Lets make threadB has higher priority than the workqueue
	 * used on device_runtime
	 */
	k_thread_create(&threadB, threadB_stack,
			K_THREAD_STACK_SIZEOF(threadB_stack),
			threadB_func, NULL, NULL, NULL,
			K_HIGHEST_THREAD_PRIO, 0, K_FOREVER);
}

void test_teardown(void)
{
	int ret;

	zassert_true(dev->pm->state == PM_DEVICE_STATE_ACTIVE, "Wrong state");

	ret = api->close_sync(dev);
	zassert_true(ret == 0, "Fail to suspend device");

	zassert_true(dev->pm->state == PM_DEVICE_STATE_SUSPEND, "Wrong state");
}

/*
 * @brief test device runtime sync API
 *
 * @details
 *  - Just bring up and put down the device using the synchronous API.
 *
 * @see pm_device_get_async(), pm_device_put()
 *
 * @ingroup power_tests
 */
void test_sync(void)
{
	int ret;

	ret = api->open_sync(dev);
	zassert_true(ret == 0, "Fail to bring up device");

	zassert_true(dev->pm->state == PM_DEVICE_STATE_ACTIVE, "Wrong state");


	ret = api->close_sync(dev);
	zassert_true(ret == 0, "Fail to suspend device");

	zassert_true(dev->pm->state == PM_DEVICE_STATE_SUSPEND, "Wrong state");
}

/*
 * @brief test device runtime async API with multiple calls to check
 * if the reference count keeps consistent.
 *
 * @ingroup power_tests
 */
void test_multiple_times(void)
{
	int ret;
	uint8_t i;

	/* First do it synchronously */
	for (i = 0; i < MAX_TIMES; i++) {
		ret = api->open_sync(dev);
		zassert_true(ret == 0, "Fail to bring up device");

		zassert_true(dev->pm->state == PM_DEVICE_STATE_ACTIVE, "Wrong state");


		ret = api->close_sync(dev);
		zassert_true(ret == 0, "Fail to suspend device");

		zassert_true(dev->pm->state == PM_DEVICE_STATE_SUSPEND, "Wrong state");
	}

	/* Now do all requests for get and then all for put*/
	for (i = 0; i < MAX_TIMES; i++) {
		ret = api->open(dev);
		zassert_true(ret == 0, "Fail to bring up device");
	}

	for (i = 0; i < MAX_TIMES; i++) {
		ret = api->close(dev);
		zassert_true(ret == 0, "Fail to suspend device");
	}

	ret = api->wait(dev);
	zassert_true(ret == 0, "Fail to wait transaction");

	/* Check the state */
	zassert_true(dev->pm->state == PM_DEVICE_STATE_SUSPEND, "Wrong state");

	/* Finally off by one to keep the device active*/
	for (i = 0; i < MAX_TIMES; i++) {
		ret = api->open(dev);
		zassert_true(ret == 0, "Fail to bring up device");
	}

	for (i = 0; i < MAX_TIMES - 1; i++) {
		ret = api->close(dev);
		zassert_true(ret == 0, "Fail to suspend device");
	}

	ret = api->wait(dev);
	zassert_true(ret == 0, "Fail to wait transaction");

	/* Check the state */
	zassert_true(dev->pm->state == PM_DEVICE_STATE_ACTIVE, "Wrong state");

}

void test_main(void)
{
	ztest_test_suite(device_runtime_test,
			 ztest_unit_test_setup_teardown(test_concurrency,
							test_setup,
							test_teardown),
			 ztest_unit_test(test_sync),
			 ztest_unit_test(test_multiple_times));
	ztest_run_test_suite(device_runtime_test);
}
