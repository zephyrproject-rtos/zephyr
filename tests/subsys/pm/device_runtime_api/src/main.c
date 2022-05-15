/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include "test_driver.h"

static const struct device *dev;
static struct k_thread get_runner_td;
K_THREAD_STACK_DEFINE(get_runner_stack, 1024);

static void get_runner(void *arg1, void *arg2, void *arg3)
{
	int ret;
	bool ongoing;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	/* make sure we test blocking path (suspend is ongoing) */
	ongoing = test_driver_pm_ongoing(dev);
	zassert_equal(ongoing, true, NULL);

	/* usage: 0, +1, resume: yes */
	ret = pm_device_runtime_get(dev);
	zassert_equal(ret, 0, NULL);
}

static void test_api_setup(void)
{
	int ret;
	enum pm_device_state state;

	/* check API always returns 0 when runtime PM is disabled */
	ret = pm_device_runtime_get(dev);
	zassert_equal(ret, 0, NULL);
	ret = pm_device_runtime_put(dev);
	zassert_equal(ret, 0, NULL);
	ret = pm_device_runtime_put_async(dev);
	zassert_equal(ret, 0, NULL);

	/* enable runtime PM */
	ret = pm_device_runtime_enable(dev);
	zassert_equal(ret, 0, NULL);

	(void)pm_device_state_get(dev, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED, NULL);

	/* enabling again should succeed (no-op) */
	ret = pm_device_runtime_enable(dev);
	zassert_equal(ret, 0, NULL);
}

static void test_api_teardown(void)
{
	int ret;
	enum pm_device_state state;

	/* let test driver finish async PM (in case it was left pending due to
	 * a failure)
	 */
	if (test_driver_pm_ongoing(dev)) {
		test_driver_pm_done(dev);
	}

	/* disable runtime PM, make sure device is left into active state */
	ret = pm_device_runtime_disable(dev);
	zassert_equal(ret, 0, NULL);

	(void)pm_device_state_get(dev, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE, NULL);
}

/**
 * @brief Test the behavior of the device runtime PM API.
 *
 * Scenarios tested:
 *
 * - get + put
 * - get + asynchronous put until suspended
 * - get + asynchronous put + get (while suspend still ongoing)
 */
static void test_api(void)
{
	int ret;
	enum pm_device_state state;

	/* device is initially suspended */
	(void)pm_device_state_get(dev, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED, NULL);

	/*** get + put ***/

	/* usage: 0, +1, resume: yes */
	ret = pm_device_runtime_get(dev);
	zassert_equal(ret, 0, NULL);

	(void)pm_device_state_get(dev, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE, NULL);

	/* usage: 1, +1, resume: no */
	ret = pm_device_runtime_get(dev);
	zassert_equal(ret, 0, NULL);

	/* usage: 2, -1, suspend: no */
	ret = pm_device_runtime_put(dev);
	zassert_equal(ret, 0, NULL);

	(void)pm_device_state_get(dev, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE, NULL);

	/* usage: 1, -1, suspend: yes */
	ret = pm_device_runtime_put(dev);
	zassert_equal(ret, 0, NULL);

	(void)pm_device_state_get(dev, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED, NULL);

	/* usage: 0, -1, suspend: no (unbalanced call) */
	ret = pm_device_runtime_put(dev);
	zassert_equal(ret, -EALREADY, NULL);

	/*** get + asynchronous put until suspended ***/

	/* usage: 0, +1, resume: yes */
	ret = pm_device_runtime_get(dev);
	zassert_equal(ret, 0, NULL);

	(void)pm_device_state_get(dev, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE, NULL);

	test_driver_pm_async(dev);

	/* usage: 1, -1, suspend: yes (queued) */
	ret = pm_device_runtime_put_async(dev);
	zassert_equal(ret, 0, NULL);

	(void)pm_device_state_get(dev, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDING, NULL);

	/* usage: 0, -1, suspend: no (unbalanced call) */
	ret = pm_device_runtime_put(dev);
	zassert_equal(ret, -EALREADY, NULL);

	/* usage: 0, -1, suspend: no (unbalanced call) */
	ret = pm_device_runtime_put_async(dev);
	zassert_equal(ret, -EALREADY, NULL);

	/* unblock test driver and let it finish */
	test_driver_pm_done(dev);
	k_yield();

	(void)pm_device_state_get(dev, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED, NULL);

	/*** get + asynchronous put + get (while suspend still ongoing) ***/

	/* usage: 0, +1, resume: yes */
	ret = pm_device_runtime_get(dev);
	zassert_equal(ret, 0, NULL);

	(void)pm_device_state_get(dev, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE, NULL);

	test_driver_pm_async(dev);

	/* usage: 1, -1, suspend: yes (queued) */
	ret = pm_device_runtime_put_async(dev);
	zassert_equal(ret, 0, NULL);

	(void)pm_device_state_get(dev, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDING, NULL);

	/* let suspension start */
	k_yield();

	/* create and start get_runner thread
	 * get_runner thread is used to test synchronous path while asynchronous
	 * is ongoing. It is important to set its priority >= to the system work
	 * queue to make sure sync path run by the thread is forced to wait.
	 */
	k_thread_create(&get_runner_td, get_runner_stack,
			K_THREAD_STACK_SIZEOF(get_runner_stack), get_runner,
			NULL, NULL, NULL, CONFIG_SYSTEM_WORKQUEUE_PRIORITY, 0,
			K_NO_WAIT);
	k_yield();

	/* let driver suspend to finish and wait until get_runner finishes
	 * resuming the driver
	 */
	test_driver_pm_done(dev);
	k_thread_join(&get_runner_td, K_FOREVER);

	(void)pm_device_state_get(dev, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE, NULL);

	/* Put operation should fail due the state be locked. */
	ret = pm_device_runtime_disable(dev);
	zassert_equal(ret, 0, NULL);

	pm_device_state_lock(dev);

	/* This operation should not succeed.  */
	ret = pm_device_runtime_enable(dev);
	zassert_equal(ret, -EPERM, NULL);

	/* After unlock the state, enable runtime should work. */
	pm_device_state_unlock(dev);

	ret = pm_device_runtime_enable(dev);
	zassert_equal(ret, 0, NULL);
}

static int pm_unsupported_init(const struct device *dev)
{
	return 0;
}

DEVICE_DEFINE(pm_unsupported_device, "PM Unsupported", pm_unsupported_init,
	      NULL, NULL, NULL, APPLICATION, 0, NULL);

static void test_unsupported(void)
{
	const struct device *dev = DEVICE_GET(pm_unsupported_device);

	zassert_false(pm_device_runtime_is_enabled(dev), "");
	zassert_equal(pm_device_runtime_enable(dev), -ENOTSUP, "");
	zassert_equal(pm_device_runtime_disable(dev), -ENOTSUP, "");
	zassert_equal(pm_device_runtime_get(dev), -ENOTSUP, "");
	zassert_equal(pm_device_runtime_put(dev), -ENOTSUP, "");
}

void test_main(void)
{
	dev = device_get_binding("test_driver");
	zassert_not_null(dev, NULL);

	ztest_test_suite(device_runtime_api,
			 ztest_unit_test_setup_teardown(test_api,
							test_api_setup,
							test_api_teardown),
			 ztest_unit_test(test_unsupported));
	ztest_run_test_suite(device_runtime_api);
}
