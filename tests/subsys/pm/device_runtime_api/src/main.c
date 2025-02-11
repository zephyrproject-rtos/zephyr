/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include "test_driver.h"

static const struct device *test_dev;
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
	ongoing = test_driver_pm_ongoing(test_dev);
	zassert_equal(ongoing, true);

	/* usage: 0, +1, resume: yes */
	ret = pm_device_runtime_get(test_dev);
	zassert_equal(ret, 0);
}

void test_api_setup(void *data)
{
	int ret;
	enum pm_device_state state;

	/* check API always returns 0 when runtime PM is disabled */
	ret = pm_device_runtime_get(test_dev);
	zassert_equal(ret, 0);
	ret = pm_device_runtime_put(test_dev);
	zassert_equal(ret, 0);
	ret = pm_device_runtime_put_async(test_dev, K_NO_WAIT);
	zassert_equal(ret, 0);

	/* enable runtime PM */
	ret = pm_device_runtime_enable(test_dev);
	zassert_equal(ret, 0);

	(void)pm_device_state_get(test_dev, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);

	/* enabling again should succeed (no-op) */
	ret = pm_device_runtime_enable(test_dev);
	zassert_equal(ret, 0);
}

static void test_api_teardown(void *data)
{
	int ret;
	enum pm_device_state state;

	/* let test driver finish async PM (in case it was left pending due to
	 * a failure)
	 */
	if (test_driver_pm_ongoing(test_dev)) {
		test_driver_pm_done(test_dev);
	}

	/* disable runtime PM, make sure device is left into active state */
	ret = pm_device_runtime_disable(test_dev);
	zassert_equal(ret, 0);

	(void)pm_device_state_get(test_dev, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE);
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
ZTEST(device_runtime_api, test_api)
{
	int ret;
	enum pm_device_state state;

	/* device is initially suspended */
	(void)pm_device_state_get(test_dev, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);
	zassert_equal(pm_device_runtime_usage(test_dev), 0);

	/*** get + put ***/

	/* usage: 0, +1, resume: yes */
	ret = pm_device_runtime_get(test_dev);
	zassert_equal(ret, 0);

	(void)pm_device_state_get(test_dev, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE);

	/* usage: 1, +1, resume: no */
	ret = pm_device_runtime_get(test_dev);
	zassert_equal(ret, 0);
	zassert_equal(pm_device_runtime_usage(test_dev), 2);

	/* usage: 2, -1, suspend: no */
	ret = pm_device_runtime_put(test_dev);
	zassert_equal(ret, 0);

	(void)pm_device_state_get(test_dev, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE);

	/* usage: 1, -1, suspend: yes */
	ret = pm_device_runtime_put(test_dev);
	zassert_equal(ret, 0);
	zassert_equal(pm_device_runtime_usage(test_dev), 0);

	(void)pm_device_state_get(test_dev, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);

	/* usage: 0, -1, suspend: no (unbalanced call) */
	ret = pm_device_runtime_put(test_dev);
	zassert_equal(ret, -EALREADY);
	zassert_equal(pm_device_runtime_usage(test_dev), 0);

	/*** get + asynchronous put until suspended ***/

	/* usage: 0, +1, resume: yes */
	ret = pm_device_runtime_get(test_dev);
	zassert_equal(ret, 0);
	zassert_equal(pm_device_runtime_usage(test_dev), 1);

	(void)pm_device_state_get(test_dev, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE);

	test_driver_pm_async(test_dev);

	/* usage: 1, -1, suspend: yes (queued) */
	ret = pm_device_runtime_put_async(test_dev, K_NO_WAIT);
	zassert_equal(ret, 0);
	zassert_equal(pm_device_runtime_usage(test_dev), 0);

	if (IS_ENABLED(CONFIG_TEST_PM_DEVICE_ISR_SAFE)) {
		/* In sync mode async put is equivalent as normal put. */
		(void)pm_device_state_get(test_dev, &state);
		zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);
		zassert_equal(pm_device_runtime_usage(test_dev), 0);
	} else {
		(void)pm_device_state_get(test_dev, &state);
		zassert_equal(state, PM_DEVICE_STATE_SUSPENDING);

		/* usage: 0, -1, suspend: no (unbalanced call) */
		ret = pm_device_runtime_put(test_dev);
		zassert_equal(ret, -EALREADY);

		/* usage: 0, -1, suspend: no (unbalanced call) */
		ret = pm_device_runtime_put_async(test_dev, K_NO_WAIT);
		zassert_equal(ret, -EALREADY);
		zassert_equal(pm_device_runtime_usage(test_dev), 0);

		/* unblock test driver and let it finish */
		test_driver_pm_done(test_dev);
		k_yield();

		(void)pm_device_state_get(test_dev, &state);
		zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);

		/*** get + asynchronous put + get (while suspend still ongoing) ***/

		/* usage: 0, +1, resume: yes */
		ret = pm_device_runtime_get(test_dev);
		zassert_equal(ret, 0);

		(void)pm_device_state_get(test_dev, &state);
		zassert_equal(state, PM_DEVICE_STATE_ACTIVE);

		test_driver_pm_async(test_dev);

		/* usage: 1, -1, suspend: yes (queued) */
		ret = pm_device_runtime_put_async(test_dev, K_NO_WAIT);
		zassert_equal(ret, 0);

		(void)pm_device_state_get(test_dev, &state);
		zassert_equal(state, PM_DEVICE_STATE_SUSPENDING);

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
		test_driver_pm_done(test_dev);
		k_thread_join(&get_runner_td, K_FOREVER);

		(void)pm_device_state_get(test_dev, &state);
		zassert_equal(state, PM_DEVICE_STATE_ACTIVE);

		/* Test if getting a device before an async operation starts does
		 * not trigger any device pm action.
		 */
		size_t count = test_driver_pm_count(test_dev);

		ret = pm_device_runtime_put_async(test_dev, K_MSEC(10));
		zassert_equal(ret, 0);

		(void)pm_device_state_get(test_dev, &state);
		zassert_equal(state, PM_DEVICE_STATE_SUSPENDING);

		ret = pm_device_runtime_get(test_dev);
		zassert_equal(ret, 0);

		/* Now lets check if the calls above have triggered a device
		 * pm action
		 */
		zassert_equal(count, test_driver_pm_count(test_dev));

		/*
		 * test if async put with a delay respects the given time.
		 */
		ret = pm_device_runtime_put_async(test_dev, K_MSEC(100));

		(void)pm_device_state_get(test_dev, &state);
		zassert_equal(state, PM_DEVICE_STATE_SUSPENDING);

		k_sleep(K_MSEC(80));

		/* It should still be suspending since we have waited less than
		 * the delay we've set.
		 */
		(void)pm_device_state_get(test_dev, &state);
		zassert_equal(state, PM_DEVICE_STATE_SUSPENDING);

		k_sleep(K_MSEC(30));

		/* Now it should be already suspended */
		(void)pm_device_state_get(test_dev, &state);
		zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);
	}

	/* Put operation should fail due the state be locked. */
	ret = pm_device_runtime_disable(test_dev);
	zassert_equal(ret, 0);
	zassert_equal(pm_device_runtime_usage(test_dev), -ENOTSUP);
}

DEVICE_DEFINE(pm_unsupported_device, "PM Unsupported", NULL, NULL, NULL, NULL,
	      POST_KERNEL, 0, NULL);

ZTEST(device_runtime_api, test_unsupported)
{
	const struct device *const dev = DEVICE_GET(pm_unsupported_device);

	zassert_false(pm_device_runtime_is_enabled(dev), "");
	zassert_equal(pm_device_runtime_enable(dev), -ENOTSUP, "");
	zassert_equal(pm_device_runtime_disable(dev), -ENOTSUP, "");
	zassert_equal(pm_device_runtime_get(dev), 0, "");
	zassert_equal(pm_device_runtime_put(dev), 0, "");
	zassert_false(pm_device_runtime_put_async(dev, K_NO_WAIT),  "");
}

int dev_pm_control(const struct device *dev, enum pm_device_action action)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(action);

	return 0;
}

PM_DEVICE_DT_DEFINE(DT_NODELABEL(test_dev), dev_pm_control);
DEVICE_DT_DEFINE(DT_NODELABEL(test_dev), NULL, PM_DEVICE_DT_GET(DT_NODELABEL(test_dev)),
		 NULL, NULL, POST_KERNEL, 80, NULL);

ZTEST(device_runtime_api, test_pm_device_runtime_auto)
{
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(test_dev));

	zassert_true(pm_device_runtime_is_enabled(dev), "");
	zassert_equal(pm_device_runtime_get(dev), 0, "");
	zassert_equal(pm_device_runtime_put(dev), 0, "");
}

void *device_runtime_api_setup(void)
{
	test_dev = device_get_binding("test_driver");
	zassert_not_null(test_dev, NULL);
	return NULL;
}

ZTEST_SUITE(device_runtime_api, NULL, device_runtime_api_setup,
			test_api_setup, test_api_teardown, NULL);
