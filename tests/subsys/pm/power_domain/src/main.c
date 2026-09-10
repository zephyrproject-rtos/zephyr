/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

/*
 * Devices used by the multiple-power-domain scenarios. Each domain (left)
 * powers the device(s) that depend on it (right). Mermaid:
 *
 *   graph TD
 *     subgraph multiple [test_power_domain_multiple: two domains + shared refcount]
 *       td1[test_domain_1] --> mult[test_dev_multi]
 *       td2[test_domain_2] --> mult
 *       td2 --> shared[test_dev_shared]
 *     end
 *     subgraph domain_fail [test_power_domain_multiple_domain_fail: rollback]
 *       f1[test_domain_f1] --> mf[test_dev_multi_fail]
 *       f2[test_domain_f2] --> mf
 *       f3[test_domain_f3] --> mf
 *       f4["test_domain_f4 (resume fails)"] --> mf
 *       f5[test_domain_f5] --> mf
 *     end
 *     subgraph turn_on_fail [test_power_domain_turn_on_fail]
 *       g[test_domain_g] --> tof["test_dev_turn_on_fail (TURN_ON fails)"]
 *     end
 *     subgraph async [test_power_domain_multiple_async]
 *       a1[test_domain_async1] --> da[test_dev_async_multi]
 *       a2[test_domain_async2] --> da
 *     end
 */

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

/* Domain that refuses to resume, used to fail a get after earlier domains
 * have already been claimed.
 */
static int fail_resume_pm_action(const struct device *dev,
				 enum pm_device_action action)
{
	ARG_UNUSED(dev);

	if (action == PM_DEVICE_ACTION_RESUME) {
		return -EIO;
	}

	return 0;
}

/* Device that refuses to power on, used to fail a get on its first domain. */
static int fail_turn_on_pm_action(const struct device *dev,
				  enum pm_device_action action)
{
	ARG_UNUSED(dev);

	if (action == PM_DEVICE_ACTION_TURN_ON) {
		return -EIO;
	}

	return 0;
}

#define TEST_DOMAIN_1 DT_NODELABEL(test_domain_1)
#define TEST_DOMAIN_2 DT_NODELABEL(test_domain_2)
#define TEST_DEV_MULTI DT_NODELABEL(test_dev_multi)
#define TEST_DEV_SHARED DT_NODELABEL(test_dev_shared)
#define TEST_DOMAIN_F1 DT_NODELABEL(test_domain_f1)
#define TEST_DOMAIN_F2 DT_NODELABEL(test_domain_f2)
#define TEST_DOMAIN_F3 DT_NODELABEL(test_domain_f3)
#define TEST_DOMAIN_F4 DT_NODELABEL(test_domain_f4)
#define TEST_DOMAIN_F5 DT_NODELABEL(test_domain_f5)
#define TEST_DEV_MULTI_FAIL DT_NODELABEL(test_dev_multi_fail)
#define TEST_DOMAIN_G DT_NODELABEL(test_domain_g)
#define TEST_DEV_TURN_ON_FAIL DT_NODELABEL(test_dev_turn_on_fail)
#define TEST_DOMAIN_ASYNC1 DT_NODELABEL(test_domain_async1)
#define TEST_DOMAIN_ASYNC2 DT_NODELABEL(test_domain_async2)
#define TEST_DEV_ASYNC_MULTI DT_NODELABEL(test_dev_async_multi)

#define TEST_ISR_SAFE COND_CODE_1(CONFIG_TEST_PM_DEVICE_ISR_SAFE, (PM_DEVICE_ISR_SAFE), (0))

PM_DEVICE_DT_DEFINE(TEST_DOMAIN_1, domain_pm_action, TEST_ISR_SAFE);
DEVICE_DT_DEFINE(TEST_DOMAIN_1, NULL, PM_DEVICE_DT_GET(TEST_DOMAIN_1),
		 NULL, NULL, POST_KERNEL, 10, NULL);

PM_DEVICE_DT_DEFINE(TEST_DOMAIN_2, domain_pm_action, TEST_ISR_SAFE);
DEVICE_DT_DEFINE(TEST_DOMAIN_2, NULL, PM_DEVICE_DT_GET(TEST_DOMAIN_2),
		 NULL, NULL, POST_KERNEL, 10, NULL);

PM_DEVICE_DT_DEFINE(TEST_DEV_MULTI, deva_pm_action, TEST_ISR_SAFE);
DEVICE_DT_DEFINE(TEST_DEV_MULTI, NULL, PM_DEVICE_DT_GET(TEST_DEV_MULTI),
		 NULL, NULL, POST_KERNEL, 20, NULL);

PM_DEVICE_DT_DEFINE(TEST_DEV_SHARED, deva_pm_action, TEST_ISR_SAFE);
DEVICE_DT_DEFINE(TEST_DEV_SHARED, NULL, PM_DEVICE_DT_GET(TEST_DEV_SHARED),
		 NULL, NULL, POST_KERNEL, 20, NULL);

PM_DEVICE_DT_DEFINE(TEST_DOMAIN_F1, domain_pm_action);
DEVICE_DT_DEFINE(TEST_DOMAIN_F1, NULL, PM_DEVICE_DT_GET(TEST_DOMAIN_F1),
		 NULL, NULL, POST_KERNEL, 10, NULL);

PM_DEVICE_DT_DEFINE(TEST_DOMAIN_F2, domain_pm_action);
DEVICE_DT_DEFINE(TEST_DOMAIN_F2, NULL, PM_DEVICE_DT_GET(TEST_DOMAIN_F2),
		 NULL, NULL, POST_KERNEL, 10, NULL);

PM_DEVICE_DT_DEFINE(TEST_DOMAIN_F3, domain_pm_action);
DEVICE_DT_DEFINE(TEST_DOMAIN_F3, NULL, PM_DEVICE_DT_GET(TEST_DOMAIN_F3),
		 NULL, NULL, POST_KERNEL, 10, NULL);

PM_DEVICE_DT_DEFINE(TEST_DOMAIN_F4, fail_resume_pm_action);
DEVICE_DT_DEFINE(TEST_DOMAIN_F4, NULL, PM_DEVICE_DT_GET(TEST_DOMAIN_F4),
		 NULL, NULL, POST_KERNEL, 10, NULL);

PM_DEVICE_DT_DEFINE(TEST_DOMAIN_F5, domain_pm_action);
DEVICE_DT_DEFINE(TEST_DOMAIN_F5, NULL, PM_DEVICE_DT_GET(TEST_DOMAIN_F5),
		 NULL, NULL, POST_KERNEL, 10, NULL);

PM_DEVICE_DT_DEFINE(TEST_DEV_MULTI_FAIL, deva_pm_action);
DEVICE_DT_DEFINE(TEST_DEV_MULTI_FAIL, NULL, PM_DEVICE_DT_GET(TEST_DEV_MULTI_FAIL),
		 NULL, NULL, POST_KERNEL, 20, NULL);

PM_DEVICE_DT_DEFINE(TEST_DOMAIN_G, domain_pm_action);
DEVICE_DT_DEFINE(TEST_DOMAIN_G, NULL, PM_DEVICE_DT_GET(TEST_DOMAIN_G),
		 NULL, NULL, POST_KERNEL, 10, NULL);

PM_DEVICE_DT_DEFINE(TEST_DEV_TURN_ON_FAIL, fail_turn_on_pm_action);
DEVICE_DT_DEFINE(TEST_DEV_TURN_ON_FAIL, NULL, PM_DEVICE_DT_GET(TEST_DEV_TURN_ON_FAIL),
		 NULL, NULL, POST_KERNEL, 20, NULL);

/* Blocking (non-ISR) so pm_device_runtime_put_async() uses the work queue. */
PM_DEVICE_DT_DEFINE(TEST_DOMAIN_ASYNC1, domain_pm_action);
DEVICE_DT_DEFINE(TEST_DOMAIN_ASYNC1, NULL, PM_DEVICE_DT_GET(TEST_DOMAIN_ASYNC1),
		 NULL, NULL, POST_KERNEL, 10, NULL);

PM_DEVICE_DT_DEFINE(TEST_DOMAIN_ASYNC2, domain_pm_action);
DEVICE_DT_DEFINE(TEST_DOMAIN_ASYNC2, NULL, PM_DEVICE_DT_GET(TEST_DOMAIN_ASYNC2),
		 NULL, NULL, POST_KERNEL, 10, NULL);

PM_DEVICE_DT_DEFINE(TEST_DEV_ASYNC_MULTI, deva_pm_action);
DEVICE_DT_DEFINE(TEST_DEV_ASYNC_MULTI, NULL, PM_DEVICE_DT_GET(TEST_DEV_ASYNC_MULTI),
		 NULL, NULL, POST_KERNEL, 20, NULL);

/**
 * @brief Test a device that depends on more than one power domain
 *
 * Scenarios tested:
 *
 * - getting the device resumes every domain it depends on
 * - the device is only reported powered when all its domains are active
 * - a domain shared with another device stays active until the last user
 *   releases it
 */
ZTEST(power_domain_1cpu, test_power_domain_multiple)
{
	const struct device *d1 = DEVICE_DT_GET(TEST_DOMAIN_1);
	const struct device *d2 = DEVICE_DT_GET(TEST_DOMAIN_2);
	const struct device *multi = DEVICE_DT_GET(TEST_DEV_MULTI);
	const struct device *shared = DEVICE_DT_GET(TEST_DEV_SHARED);
	enum pm_device_state s1, s2;
	int ret;

	pm_device_init_suspended(d1);
	pm_device_init_suspended(d2);
	pm_device_init_suspended(multi);
	pm_device_init_suspended(shared);
	pm_device_runtime_enable(d1);
	pm_device_runtime_enable(d2);
	pm_device_runtime_enable(multi);
	pm_device_runtime_enable(shared);

	/* Both domains are suspended, so the device is not powered yet. */
	zassert_false(pm_device_is_powered(multi));

	/* Getting the device must resume both domains. */
	ret = pm_device_runtime_get(multi);
	zassert_equal(ret, 0);
	zassert_true(atomic_test_bit(&multi->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));

	pm_device_state_get(d1, &s1);
	pm_device_state_get(d2, &s2);
	zassert_equal(s1, PM_DEVICE_STATE_ACTIVE);
	zassert_equal(s2, PM_DEVICE_STATE_ACTIVE);
	zassert_true(pm_device_is_powered(multi));

	/* A second device also needs domain 2. */
	ret = pm_device_runtime_get(shared);
	zassert_equal(ret, 0);

	/*
	 * Releasing the multi-domain device powers domain 1 down but keeps
	 * domain 2 active, since the shared device still needs it.
	 */
	ret = pm_device_runtime_put(multi);
	zassert_equal(ret, 0);
	zassert_false(atomic_test_bit(&multi->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));

	pm_device_state_get(d1, &s1);
	pm_device_state_get(d2, &s2);
	zassert_equal(s1, PM_DEVICE_STATE_SUSPENDED);
	zassert_equal(s2, PM_DEVICE_STATE_ACTIVE);

	/* The last user releases domain 2 as well. */
	ret = pm_device_runtime_put(shared);
	zassert_equal(ret, 0);

	pm_device_state_get(d2, &s2);
	zassert_equal(s2, PM_DEVICE_STATE_SUSPENDED);
}

/**
 * @brief Test that a get failing partway through rolls back the claimed domains
 *
 * The device depends on five domains and the fourth one refuses to resume, so
 * the get must release the three domains already claimed, never reach the last
 * one, and leave the device unclaimed.
 */
ZTEST(power_domain_1cpu, test_power_domain_multiple_domain_fail)
{
	const struct device *f[] = {
		DEVICE_DT_GET(TEST_DOMAIN_F1), DEVICE_DT_GET(TEST_DOMAIN_F2),
		DEVICE_DT_GET(TEST_DOMAIN_F3), DEVICE_DT_GET(TEST_DOMAIN_F4),
		DEVICE_DT_GET(TEST_DOMAIN_F5),
	};
	const struct device *multi = DEVICE_DT_GET(TEST_DEV_MULTI_FAIL);
	enum pm_device_state state;
	int ret;

	for (size_t i = 0U; i < ARRAY_SIZE(f); i++) {
		pm_device_init_suspended(f[i]);
		pm_device_runtime_enable(f[i]);
	}
	pm_device_init_suspended(multi);
	pm_device_runtime_enable(multi);

	ret = pm_device_runtime_get(multi);
	zassert_not_equal(ret, 0);
	zassert_false(atomic_test_bit(&multi->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));

	/* Every domain must be back to (or still) suspended. */
	for (size_t i = 0U; i < ARRAY_SIZE(f); i++) {
		pm_device_state_get(f[i], &state);
		zassert_equal(state, PM_DEVICE_STATE_SUSPENDED, "domain f%d not suspended",
			      (int)i + 1);
	}
}

/**
 * @brief Test rollback when the device itself refuses to power on
 *
 * The domain resumes but the device's TURN_ON fails, so the get must fail and
 * release the domain again.
 */
ZTEST(power_domain_1cpu, test_power_domain_turn_on_fail)
{
	const struct device *g = DEVICE_DT_GET(TEST_DOMAIN_G);
	const struct device *dev = DEVICE_DT_GET(TEST_DEV_TURN_ON_FAIL);
	enum pm_device_state state;
	int ret;

	pm_device_init_suspended(g);
	pm_device_init_suspended(dev);
	pm_device_runtime_enable(g);
	pm_device_runtime_enable(dev);

	/*
	 * TURN_ON only runs from the OFF state, so cut the device's power once
	 * to drive it OFF. Its next power-up then actually invokes (and fails)
	 * the TURN_ON callback.
	 */
	pm_device_runtime_get(g);
	pm_device_runtime_put(g);

	ret = pm_device_runtime_get(dev);
	zassert_not_equal(ret, 0);
	zassert_false(atomic_test_bit(&dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));

	pm_device_state_get(g, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);
}

#ifdef CONFIG_PM_DEVICE_RUNTIME_ASYNC
/**
 * @brief Test asynchronous release of a device on multiple power domains
 *
 * pm_device_runtime_put_async() defers the suspend to a work item; once it
 * runs, every domain the device depends on must be released.
 */
ZTEST(power_domain, test_power_domain_multiple_async)
{
	const struct device *d1 = DEVICE_DT_GET(TEST_DOMAIN_ASYNC1);
	const struct device *d2 = DEVICE_DT_GET(TEST_DOMAIN_ASYNC2);
	const struct device *dev = DEVICE_DT_GET(TEST_DEV_ASYNC_MULTI);
	enum pm_device_state s1, s2, sd;
	int ret;

	pm_device_init_suspended(d1);
	pm_device_init_suspended(d2);
	pm_device_init_suspended(dev);
	pm_device_runtime_enable(d1);
	pm_device_runtime_enable(d2);
	pm_device_runtime_enable(dev);

	ret = pm_device_runtime_get(dev);
	zassert_equal(ret, 0);
	pm_device_state_get(d1, &s1);
	pm_device_state_get(d2, &s2);
	zassert_equal(s1, PM_DEVICE_STATE_ACTIVE);
	zassert_equal(s2, PM_DEVICE_STATE_ACTIVE);

	/* Defer the suspend; the work item must release both domains. */
	ret = pm_device_runtime_put_async(dev, K_NO_WAIT);
	zassert_equal(ret, 0);

	/* Let the suspend work run. */
	k_sleep(K_MSEC(100));

	/* Both domains must have been released and powered down. */
	zassert_false(atomic_test_bit(&dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));
	pm_device_state_get(d1, &s1);
	pm_device_state_get(d2, &s2);
	zassert_equal(s1, PM_DEVICE_STATE_SUSPENDED, "d1=%d", s1);
	zassert_equal(s2, PM_DEVICE_STATE_SUSPENDED, "d2=%d", s2);

	/* Powering the domains off notifies the device, so it ends OFF. */
	pm_device_state_get(dev, &sd);
	zassert_equal(sd, PM_DEVICE_STATE_OFF, "dev=%d", sd);
}

/*
 * Separate suite: the async suspend runs on a work queue, so it needs the
 * scheduler live and must not use the scheduler-locked 1cpu harness.
 */
ZTEST_SUITE(power_domain, NULL, NULL, NULL, NULL, NULL);
#endif /* CONFIG_PM_DEVICE_RUNTIME_ASYNC */

#define TEST_DOMAIN_DIS1 DT_NODELABEL(test_domain_dis1)
#define TEST_DOMAIN_DIS3 DT_NODELABEL(test_domain_dis3)
#define TEST_DEV_DIS DT_NODELABEL(test_dev_dis)

PM_DEVICE_DT_DEFINE(TEST_DOMAIN_DIS1, domain_pm_action);
DEVICE_DT_DEFINE(TEST_DOMAIN_DIS1, NULL, PM_DEVICE_DT_GET(TEST_DOMAIN_DIS1),
		 NULL, NULL, POST_KERNEL, 10, NULL);

PM_DEVICE_DT_DEFINE(TEST_DOMAIN_DIS3, domain_pm_action);
DEVICE_DT_DEFINE(TEST_DOMAIN_DIS3, NULL, PM_DEVICE_DT_GET(TEST_DOMAIN_DIS3),
		 NULL, NULL, POST_KERNEL, 10, NULL);

PM_DEVICE_DT_DEFINE(TEST_DEV_DIS, deva_pm_action);
DEVICE_DT_DEFINE(TEST_DEV_DIS, NULL, PM_DEVICE_DT_GET(TEST_DEV_DIS),
		 NULL, NULL, POST_KERNEL, 20, NULL);

/**
 * @brief Test that a disabled domain in the list is skipped, not truncating
 *
 * The device depends on three domains and the middle one is disabled. Getting
 * the device must still resume the first and the last enabled domain.
 */
ZTEST(power_domain_1cpu, test_power_domain_disabled_domain_skipped)
{
	const struct device *d1 = DEVICE_DT_GET(TEST_DOMAIN_DIS1);
	const struct device *d3 = DEVICE_DT_GET(TEST_DOMAIN_DIS3);
	const struct device *dev = DEVICE_DT_GET(TEST_DEV_DIS);
	enum pm_device_state s1, s3;
	int ret;

	pm_device_init_suspended(d1);
	pm_device_init_suspended(d3);
	pm_device_init_suspended(dev);
	pm_device_runtime_enable(d1);
	pm_device_runtime_enable(d3);
	pm_device_runtime_enable(dev);

	ret = pm_device_runtime_get(dev);
	zassert_equal(ret, 0);

	/* The last domain must not have been dropped by the disabled one. */
	pm_device_state_get(d1, &s1);
	pm_device_state_get(d3, &s3);
	zassert_equal(s1, PM_DEVICE_STATE_ACTIVE);
	zassert_equal(s3, PM_DEVICE_STATE_ACTIVE);

	ret = pm_device_runtime_put(dev);
	zassert_equal(ret, 0);
	pm_device_state_get(d1, &s1);
	pm_device_state_get(d3, &s3);
	zassert_equal(s1, PM_DEVICE_STATE_SUSPENDED);
	zassert_equal(s3, PM_DEVICE_STATE_SUSPENDED);
}

ZTEST_SUITE(power_domain_1cpu, NULL, NULL, ztest_simple_1cpu_before,
			ztest_simple_1cpu_after, NULL);
