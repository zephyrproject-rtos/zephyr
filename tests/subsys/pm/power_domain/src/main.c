/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#define NUMBER_OF_DEVICES 3

#define TEST_DOMAIN DT_NODELABEL(test_domain)
#define TEST_DEVA DT_NODELABEL(test_dev_a)
#define TEST_DEVB DT_NODELABEL(test_dev_b)

static const struct device *const domain = DEVICE_DT_GET(TEST_DOMAIN);
static const struct device *const deva = DEVICE_DT_GET(TEST_DEVA);
static const struct device *const devb = DEVICE_DT_GET(TEST_DEVB);
static const struct device *devc;
static int testing_domain_on_notitication;
static int testing_domain_off_notitication;

static int domain_pm_action(const struct device *dev,
	enum pm_device_action action)
{
	int rc = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/* Switch power on */
		pm_device_children_action_run(dev, PM_DEVICE_ACTION_TURN_ON, NULL);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		pm_device_children_action_run(dev, PM_DEVICE_ACTION_TURN_OFF, NULL);
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		__fallthrough;
	case PM_DEVICE_ACTION_TURN_OFF:
		break;
	default:
		rc = -ENOTSUP;
	}

	return rc;

}

static int deva_pm_action(const struct device *dev,
		     enum pm_device_action pm_action)
{
	ARG_UNUSED(dev);

	if (testing_domain_on_notitication > 0) {
		if (pm_action == PM_DEVICE_ACTION_TURN_ON) {
			testing_domain_on_notitication--;
		}
	} else if (testing_domain_off_notitication > 0) {
		if (pm_action == PM_DEVICE_ACTION_TURN_OFF) {
			testing_domain_off_notitication--;
		}
	}

	return 0;
}

/*
 * Device B will return -ENOTSUP for TURN_ON and TURN_OFF actions.
 * This way we can check if the subsystem properly handled its state.
 */
static int devb_pm_action(const struct device *dev,
		     enum pm_device_action pm_action)
{
	int ret = 0;

	ARG_UNUSED(dev);

	if (testing_domain_on_notitication > 0) {
		if (pm_action == PM_DEVICE_ACTION_TURN_ON) {
			ret = -ENOTSUP;
			testing_domain_on_notitication--;
		}
	} else if (testing_domain_off_notitication > 0) {
		if (pm_action == PM_DEVICE_ACTION_TURN_OFF) {
			ret = -ENOTSUP;
			testing_domain_off_notitication--;
		}
	}

	return ret;
}


PM_DEVICE_DT_DEFINE(TEST_DOMAIN, domain_pm_action,
	COND_CODE_1(CONFIG_TEST_PM_DEVICE_ISR_SAFE, (PM_DEVICE_ISR_SAFE), (0)));

DEVICE_DT_DEFINE(TEST_DOMAIN, NULL, PM_DEVICE_DT_GET(TEST_DOMAIN),
		 NULL, NULL, POST_KERNEL, 10, NULL);

PM_DEVICE_DT_DEFINE(TEST_DEVA, deva_pm_action,
	COND_CODE_1(CONFIG_TEST_PM_DEVICE_ISR_SAFE, (PM_DEVICE_ISR_SAFE), (0)));
DEVICE_DT_DEFINE(TEST_DEVA, NULL, PM_DEVICE_DT_GET(TEST_DEVA),
		 NULL, NULL, POST_KERNEL, 20, NULL);

PM_DEVICE_DT_DEFINE(TEST_DEVB, devb_pm_action);
DEVICE_DT_DEFINE(TEST_DEVB, NULL, PM_DEVICE_DT_GET(TEST_DEVB),
		 NULL, NULL, POST_KERNEL, 30, NULL);

PM_DEVICE_DEFINE(devc, deva_pm_action);
DEVICE_DEFINE(devc, "devc", NULL, PM_DEVICE_GET(devc),
	      NULL, NULL, POST_KERNEL, 40, NULL);

/**
 * @brief Test the power domain behavior
 *
 * Scenarios tested:
 *
 * - get + put multiple devices under a domain
 * - notification when domain state changes
 */
ZTEST(power_domain_1cpu, test_power_domain_device_runtime)
{
	int ret;
	enum pm_device_state state;

	devc = DEVICE_GET(devc);

	pm_device_init_suspended(domain);
	pm_device_init_suspended(deva);
	pm_device_init_suspended(devb);
	pm_device_init_suspended(devc);

	pm_device_runtime_enable(domain);
	pm_device_runtime_enable(deva);
	pm_device_runtime_enable(devb);
	pm_device_runtime_enable(devc);

	ret = pm_device_power_domain_remove(devc, domain);
	zassert_equal(ret, -ENOENT);

	ret = pm_device_power_domain_add(devc, domain);
	zassert_equal(ret, 0);

	/* At this point all devices should be SUSPENDED */
	pm_device_state_get(domain, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);

	pm_device_state_get(deva, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);

	pm_device_state_get(devb, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);

	pm_device_state_get(devc, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);

	/* Now test if "get" a device will resume the domain */
	ret = pm_device_runtime_get(deva);
	zassert_equal(ret, 0);

	pm_device_state_get(deva, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE);
	zassert_true(atomic_test_bit(&deva->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));

	pm_device_state_get(domain, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE);

	ret = pm_device_runtime_get(devc);
	zassert_equal(ret, 0);
	zassert_true(atomic_test_bit(&devc->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));

	ret = pm_device_runtime_get(devb);
	zassert_equal(ret, 0);
	zassert_true(atomic_test_bit(&devb->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));

	ret = pm_device_runtime_put(deva);
	zassert_equal(ret, 0);
	zassert_false(atomic_test_bit(&deva->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));

	/*
	 * The domain has to still be active since device B
	 * is still in use.
	 */
	pm_device_state_get(domain, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE);

	/*
	 * Now the domain should be suspended since there is no
	 * one using it.
	 */
	ret = pm_device_runtime_put(devb);
	zassert_equal(ret, 0);
	zassert_false(atomic_test_bit(&devb->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));

	ret = pm_device_runtime_put(devc);
	zassert_equal(ret, 0);
	zassert_false(atomic_test_bit(&devc->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));

	pm_device_state_get(domain, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);

	/*
	 * With the domain suspended the device state should be OFF, since
	 * the power was completely cut.
	 */
	pm_device_state_get(devb, &state);
	zassert_equal(state, PM_DEVICE_STATE_OFF);

	pm_device_state_get(deva, &state);
	zassert_equal(state, PM_DEVICE_STATE_OFF);

	/*
	 * Now lets test that devices are notified when the domain
	 * changes its state.
	 */

	/* Three devices has to get the notification */
	testing_domain_on_notitication = NUMBER_OF_DEVICES;
	ret = pm_device_runtime_get(domain);
	zassert_equal(ret, 0);

	zassert_equal(testing_domain_on_notitication, 0);

	testing_domain_off_notitication = NUMBER_OF_DEVICES;
	ret = pm_device_runtime_put(domain);
	zassert_equal(ret, 0);

	zassert_equal(testing_domain_off_notitication, 0);

	ret = pm_device_power_domain_remove(devc, domain);
	zassert_equal(ret, 0);
}

#define TEST_DOMAIN_BALANCED DT_NODELABEL(test_domain_balanced)
#define TEST_DEV_BALANCED DT_NODELABEL(test_dev_balanced)

PM_DEVICE_DT_DEFINE(TEST_DOMAIN_BALANCED, domain_pm_action);
DEVICE_DT_DEFINE(TEST_DOMAIN_BALANCED, NULL, PM_DEVICE_DT_GET(TEST_DOMAIN_BALANCED),
		 NULL, NULL, POST_KERNEL, 10, NULL);

PM_DEVICE_DT_DEFINE(TEST_DEV_BALANCED, deva_pm_action);
DEVICE_DT_DEFINE(TEST_DEV_BALANCED, NULL, PM_DEVICE_DT_GET(TEST_DEV_BALANCED),
		 NULL, NULL, POST_KERNEL, 20, NULL);

/**
 * @brief Test power domain requests are balanced
 *
 * Scenarios tested:
 *
 * - get + put device with a PD while PM is disabled
 */
ZTEST(power_domain_1cpu, test_power_domain_device_balanced)
{
	const struct device *balanced_domain = DEVICE_DT_GET(TEST_DOMAIN_BALANCED);
	const struct device *dev = DEVICE_DT_GET(TEST_DEV_BALANCED);
	enum pm_device_state state;
	int ret;

	/* Init domain */
	pm_device_init_suspended(balanced_domain);
	pm_device_runtime_enable(balanced_domain);

	/* At this point domain should be SUSPENDED */
	pm_device_state_get(balanced_domain, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);

	/* Get and put the device without PM enabled should not change the domain */
	ret = pm_device_runtime_get(dev);
	zassert_equal(ret, 0);
	ret = pm_device_runtime_put(dev);
	zassert_equal(ret, 0);

	pm_device_state_get(balanced_domain, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);

	/* Same thing with the domain in active state */
	ret = pm_device_runtime_get(balanced_domain);
	zassert_equal(ret, 0);
	pm_device_state_get(balanced_domain, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE);

	ret = pm_device_runtime_get(dev);
	zassert_equal(ret, 0);
	ret = pm_device_runtime_put(dev);
	zassert_equal(ret, 0);

	pm_device_state_get(balanced_domain, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE);
}


/* Devices and synchronization used to force get-vs-async-suspend ordering. */
#define TEST_DOMAIN_ASYNC DT_NODELABEL(test_domain_async)
#define TEST_DEV_ASYNC    DT_NODELABEL(test_dev_async)

static bool block_async_suspend;
static K_SEM_DEFINE(async_suspend_started, 0, 1);
static K_SEM_DEFINE(async_suspend_continue, 0, 1);
static K_SEM_DEFINE(async_get_started, 0, 1);
static struct k_thread async_get_thread;
K_THREAD_STACK_DEFINE(async_get_stack, 1024);
static int domain_async_resume_count;
static int domain_async_suspend_count;

static const struct device *const domain_async = DEVICE_DT_GET(TEST_DOMAIN_ASYNC);
static const struct device *const dev_async = DEVICE_DT_GET(TEST_DEV_ASYNC);

static int domain_async_pm_action(const struct device *dev, enum pm_device_action pm_action)
{
	if (pm_action == PM_DEVICE_ACTION_RESUME) {
		domain_async_resume_count++;
	} else if (pm_action == PM_DEVICE_ACTION_SUSPEND) {
		domain_async_suspend_count++;
	}

	return domain_pm_action(dev, pm_action);
}

static int async_pm_action(const struct device *dev, enum pm_device_action pm_action)
{
	/* Hold the device suspend action open after async suspend has started. */
	if ((pm_action == PM_DEVICE_ACTION_SUSPEND) && block_async_suspend) {
		k_sem_give(&async_suspend_started);
		k_sem_take(&async_suspend_continue, K_FOREVER);
		block_async_suspend = false;
	}

	return deva_pm_action(dev, pm_action);
}

PM_DEVICE_DT_DEFINE(TEST_DOMAIN_ASYNC, domain_async_pm_action);
DEVICE_DT_DEFINE(TEST_DOMAIN_ASYNC, NULL, PM_DEVICE_DT_GET(TEST_DOMAIN_ASYNC), NULL, NULL,
		 POST_KERNEL, 10, NULL);

PM_DEVICE_DT_DEFINE(TEST_DEV_ASYNC, async_pm_action);
DEVICE_DT_DEFINE(TEST_DEV_ASYNC, NULL, PM_DEVICE_DT_GET(TEST_DEV_ASYNC), NULL, NULL, POST_KERNEL,
		 20, NULL);

static void get_async_device(void *arg1, void *arg2, void *arg3)
{
	int ret;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	k_sem_give(&async_get_started);
	ret = pm_device_runtime_get(dev_async);
	zassert_equal(ret, 0);
}

ZTEST(power_domain_1cpu, test_power_domain_get_while_async_suspend)
{
	enum pm_device_state state;
	int ret;

	pm_device_init_suspended(domain_async);
	pm_device_init_suspended(dev_async);
	domain_async_resume_count = 0;
	domain_async_suspend_count = 0;

	ret = pm_device_runtime_enable(domain_async);
	zassert_equal(ret, 0);
	ret = pm_device_runtime_enable(dev_async);
	zassert_equal(ret, 0);

	/* Start from an active child that has claimed its power domain. */
	ret = pm_device_runtime_get(dev_async);
	zassert_equal(ret, 0);
	zassert_true(pm_device_is_powered(dev_async));
	zassert_equal(domain_async_resume_count, 1);
	zassert_equal(domain_async_suspend_count, 0);

	k_sem_reset(&async_suspend_started);
	k_sem_reset(&async_suspend_continue);
	k_sem_reset(&async_get_started);
	block_async_suspend = true;

	/* Queue async suspend and stop it inside the child suspend callback. */
	ret = pm_device_runtime_put_async(dev_async, K_NO_WAIT);
	zassert_equal(ret, 0);
	zassert_equal(k_sem_take(&async_suspend_started, K_SECONDS(1)), 0);

	pm_device_state_get(dev_async, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDING);

	/* Force a resume request to arrive while the async suspend is running. */
	k_thread_create(&async_get_thread, async_get_stack, K_THREAD_STACK_SIZEOF(async_get_stack),
			get_async_device, NULL, NULL, NULL,
			K_PRIO_PREEMPT(CONFIG_SYSTEM_WORKQUEUE_PRIORITY), 0, K_NO_WAIT);

	zassert_equal(k_sem_take(&async_get_started, K_SECONDS(1)), 0);

	/* Keep the suspend blocked for a short duration while get() is running. */
	k_sleep(K_MSEC(100));

	/* Let suspend finish */
	k_sem_give(&async_suspend_continue);
	zassert_equal(k_thread_join(&async_get_thread, K_SECONDS(1)), 0);
	k_sleep(K_MSEC(10));

	/* The child must be active and must keep the domain powered/refcounted. */
	pm_device_state_get(dev_async, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE);
	pm_device_state_get(domain_async, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE);
	zassert_true(pm_device_is_powered(dev_async));
	zassert_true(atomic_test_bit(&dev_async->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));
	zassert_equal(pm_device_runtime_usage(dev_async), 1);
	zassert_equal(pm_device_runtime_usage(domain_async), 1);

	/* The domain should not have cycled */
	zassert_equal(domain_async_resume_count, 1);
	zassert_equal(domain_async_suspend_count, 0);

	ret = pm_device_runtime_put(dev_async);
	zassert_equal(ret, 0);
}

ZTEST(power_domain_1cpu, test_on_power_domain)
{
	zassert_true(device_is_ready(domain), "Device is not ready!");
	zassert_true(device_is_ready(deva), "Device is not ready!");
	devc = DEVICE_GET(devc);
	zassert_true(device_is_ready(devc), "Device is not ready!");

	pm_device_power_domain_remove(deva, domain);
	zassert_false(pm_device_on_power_domain(deva), "deva is in the power domain.");
	pm_device_power_domain_add(deva, domain);
	zassert_true(pm_device_on_power_domain(deva), "deva is not in the power domain.");

	pm_device_power_domain_add(devc, domain);
	zassert_true(pm_device_on_power_domain(devc), "devc is not in the power domain.");
	pm_device_power_domain_remove(devc, domain);
	zassert_false(pm_device_on_power_domain(devc), "devc in the power domain.");
}

ZTEST(power_domain_1cpu, test_power_domain_add_remove_duplicate)
{
	int ret;

	devc = DEVICE_GET(devc);
	zassert_true(device_is_ready(devc), "Device is not ready!");

	ret = pm_device_power_domain_remove(devc, domain);
	zassert_equal(ret, -ENOENT);

	ret = pm_device_power_domain_add(devc, domain);
	zassert_equal(ret, 0);
	zassert_true(pm_device_on_power_domain(devc), "devc is not in the power domain.");

	ret = pm_device_power_domain_add(devc, domain);
	zassert_equal(ret, -EALREADY);
	zassert_true(pm_device_on_power_domain(devc), "devc is not in the power domain.");

	ret = pm_device_power_domain_remove(devc, domain);
	zassert_equal(ret, 0);
	zassert_false(pm_device_on_power_domain(devc), "devc in the power domain.");

	ret = pm_device_power_domain_remove(devc, domain);
	zassert_equal(ret, -ENOENT);
}

ZTEST_SUITE(power_domain_1cpu, NULL, NULL, ztest_simple_1cpu_before,
			ztest_simple_1cpu_after, NULL);
