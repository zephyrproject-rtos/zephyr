/*
 * Copyright (c) 2021 Intel Corporation.
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/device_runtime_internal.h>

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

#define TEST_DOMAIN_HELPER DT_NODELABEL(test_domain_helper)
#define TEST_DEV_HELPER    DT_NODELABEL(test_dev_helper)

static atomic_t helper_domain_suspend_error;
static atomic_t helper_domain_suspend_error_after_action;
static atomic_t helper_domain_block_suspend;
static atomic_t helper_domain_block_after_action;
static atomic_t helper_direct_turn_off_claimed;
static atomic_t helper_direct_turn_off_releasing;
static atomic_t helper_action_count[PM_DEVICE_ACTION_TURN_ON + 1];
static atomic_t helper_action_error[PM_DEVICE_ACTION_TURN_ON + 1];
static int helper_get_ret;
static int helper_disable_ret;
K_SEM_DEFINE(helper_domain_suspend_entered, 0, 1);
K_SEM_DEFINE(helper_domain_suspend_continue, 0, 1);
K_SEM_DEFINE(helper_get_started, 0, 1);
K_SEM_DEFINE(helper_get_done, 0, 1);
K_SEM_DEFINE(helper_disable_started, 0, 1);
K_SEM_DEFINE(helper_disable_done, 0, 1);
K_THREAD_STACK_DEFINE(helper_get_stack, 1024);
K_THREAD_STACK_DEFINE(helper_disable_stack, 1024);
static struct k_thread helper_get_thread;
static struct k_thread helper_disable_thread;

static int helper_domain_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct device *helper_dev = DEVICE_DT_GET(TEST_DEV_HELPER);
	int ret = atomic_get(&helper_domain_suspend_error);

	if ((action == PM_DEVICE_ACTION_SUSPEND) && atomic_get(&helper_domain_block_suspend)) {
		k_sem_give(&helper_domain_suspend_entered);
		(void)k_sem_take(&helper_domain_suspend_continue, K_FOREVER);
	}
	if ((action == PM_DEVICE_ACTION_SUSPEND) && (ret != 0)) {
		return ret;
	}

	ret = domain_pm_action(dev, action);
	if ((action == PM_DEVICE_ACTION_SUSPEND) && atomic_get(&helper_domain_block_after_action)) {
		atomic_set(&helper_direct_turn_off_claimed,
			   atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));
		atomic_set(
			&helper_direct_turn_off_releasing,
			atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_RELEASING));
		k_sem_give(&helper_domain_suspend_entered);
		(void)k_sem_take(&helper_domain_suspend_continue, K_FOREVER);
	}
	if ((action == PM_DEVICE_ACTION_SUSPEND) &&
	    (atomic_get(&helper_domain_suspend_error_after_action) != 0)) {
		return atomic_get(&helper_domain_suspend_error_after_action);
	}
	if ((action == PM_DEVICE_ACTION_SUSPEND) && atomic_get(&helper_domain_block_suspend)) {
		atomic_set(&helper_direct_turn_off_claimed,
			   atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));
		atomic_set(
			&helper_direct_turn_off_releasing,
			atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_RELEASING));
	}

	return ret;
}

static int helper_dev_pm_action(const struct device *dev, enum pm_device_action action)
{
	ARG_UNUSED(dev);

	atomic_inc(&helper_action_count[action]);
	return atomic_get(&helper_action_error[action]);
}

static void helper_actions_reset(void)
{
	ARRAY_FOR_EACH(helper_action_count, i) {
		atomic_clear(&helper_action_count[i]);
		atomic_clear(&helper_action_error[i]);
	}
}

static void helper_runtime_get(void *dev, void *arg2, void *arg3)
{
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	k_sem_give(&helper_get_started);
	helper_get_ret = pm_device_runtime_get(dev);
	k_sem_give(&helper_get_done);
}

static void helper_runtime_disable(void *dev, void *arg2, void *arg3)
{
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	k_sem_give(&helper_disable_started);
	helper_disable_ret = pm_device_runtime_disable(dev);
	k_sem_give(&helper_disable_done);
}

PM_DEVICE_DT_DEFINE(TEST_DOMAIN_HELPER, helper_domain_pm_action,
		    COND_CODE_1(CONFIG_TEST_PM_DEVICE_ISR_SAFE, (PM_DEVICE_ISR_SAFE), (0)));
DEVICE_DT_DEFINE(TEST_DOMAIN_HELPER, NULL, PM_DEVICE_DT_GET(TEST_DOMAIN_HELPER), NULL, NULL,
		 POST_KERNEL, 10, NULL);

PM_DEVICE_DT_DEFINE(TEST_DEV_HELPER, helper_dev_pm_action,
		    COND_CODE_1(CONFIG_TEST_PM_DEVICE_ISR_SAFE, (PM_DEVICE_ISR_SAFE), (0)));
DEVICE_DT_DEFINE(TEST_DEV_HELPER, NULL, PM_DEVICE_DT_GET(TEST_DEV_HELPER), NULL, NULL, POST_KERNEL,
		 20, NULL);

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

ZTEST(power_domain_1cpu, test_power_domain_action_claim)
{
	const struct device *helper_domain = DEVICE_DT_GET(TEST_DOMAIN_HELPER);
	const struct device *helper_dev = DEVICE_DT_GET(TEST_DEV_HELPER);
	atomic_val_t suspend_count;
	atomic_val_t turn_off_count;
	enum pm_device_state state;

	helper_actions_reset();
	atomic_clear(&helper_domain_suspend_error);
	atomic_clear(&helper_domain_suspend_error_after_action);
	pm_device_init_suspended(helper_domain);
	pm_device_init_suspended(helper_dev);
	zassert_ok(pm_device_runtime_enable(helper_domain));
	zassert_ok(pm_device_runtime_enable(helper_dev));

	zassert_ok(pm_device_runtime_get(helper_dev));
	zassert_true(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));
	zassert_equal(pm_device_runtime_usage(helper_domain), 1);

	atomic_set(&helper_domain_suspend_error_after_action, -EIO);
	zassert_equal(pm_device_runtime_put(helper_dev), -EIO);
	zassert_equal(pm_device_runtime_usage(helper_dev), 0);
	zassert_equal(pm_device_runtime_usage(helper_domain), 1);
	zassert_true(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));
	zassert_ok(pm_device_state_get(helper_dev, &state));
	zassert_equal(state, PM_DEVICE_STATE_OFF);
	atomic_clear(&helper_domain_suspend_error_after_action);
	zassert_ok(
		z_pm_device_runtime_power_domain_action_run(helper_dev, PM_DEVICE_ACTION_TURN_ON));
	zassert_ok(pm_device_runtime_get(helper_dev));
	zassert_equal(pm_device_runtime_usage(helper_domain), 1);
	zassert_true(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));

	zassert_ok(
		z_pm_device_runtime_power_domain_action_run(helper_dev, PM_DEVICE_ACTION_TURN_OFF));
	zassert_ok(pm_device_state_get(helper_dev, &state));
	zassert_equal(state, PM_DEVICE_STATE_OFF);
	zassert_equal(pm_device_runtime_usage(helper_dev), 0);
	zassert_false(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));
	zassert_equal(pm_device_runtime_usage(helper_domain), 0);

	zassert_ok(
		z_pm_device_runtime_power_domain_action_run(helper_dev, PM_DEVICE_ACTION_TURN_ON));
	zassert_ok(pm_device_runtime_get(helper_dev));
	zassert_true(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));
	zassert_equal(pm_device_runtime_usage(helper_domain), 1);

	zassert_ok(pm_device_runtime_put(helper_dev));

	zassert_ok(
		z_pm_device_runtime_power_domain_action_run(helper_dev, PM_DEVICE_ACTION_TURN_ON));
	zassert_ok(pm_device_runtime_get(helper_dev));
	atomic_set(&helper_domain_suspend_error, -EIO);
	zassert_equal(
		z_pm_device_runtime_power_domain_action_run(helper_dev, PM_DEVICE_ACTION_TURN_OFF),
		-EIO);
	zassert_ok(pm_device_state_get(helper_dev, &state));
	zassert_equal(state, PM_DEVICE_STATE_OFF);
	zassert_equal(pm_device_runtime_usage(helper_dev), 0);
	zassert_true(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));
	zassert_false(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_RELEASING));
	zassert_equal(pm_device_runtime_usage(helper_domain), 1);
	zassert_ok(pm_device_state_get(helper_domain, &state));
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE);

	atomic_set(&helper_action_error[PM_DEVICE_ACTION_TURN_ON], -EFAULT);
	zassert_equal(
		z_pm_device_runtime_power_domain_action_run(helper_dev, PM_DEVICE_ACTION_TURN_ON),
		-EFAULT);
	zassert_equal(pm_device_runtime_get(helper_dev), -EAGAIN);
	zassert_true(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));
	zassert_equal(pm_device_runtime_usage(helper_domain), 1);
	atomic_clear(&helper_action_error[PM_DEVICE_ACTION_TURN_ON]);
	zassert_equal(
		z_pm_device_runtime_power_domain_action_run(helper_dev, PM_DEVICE_ACTION_TURN_OFF),
		-EIO);

	zassert_ok(
		z_pm_device_runtime_power_domain_action_run(helper_dev, PM_DEVICE_ACTION_TURN_ON));
	zassert_equal(pm_device_runtime_usage(helper_domain), 1);
	atomic_set(&helper_action_error[PM_DEVICE_ACTION_RESUME], -EFAULT);
	zassert_equal(pm_device_runtime_get(helper_dev), -EFAULT);
	zassert_equal(pm_device_runtime_usage(helper_dev), 0);
	zassert_equal(pm_device_runtime_usage(helper_domain), 1);
	zassert_true(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));
	atomic_clear(&helper_action_error[PM_DEVICE_ACTION_RESUME]);
	zassert_ok(pm_device_runtime_get(helper_dev));
	zassert_equal(pm_device_runtime_usage(helper_domain), 1);
	zassert_true(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));

	zassert_equal(
		z_pm_device_runtime_power_domain_action_run(helper_dev, PM_DEVICE_ACTION_TURN_OFF),
		-EIO);
	suspend_count = atomic_get(&helper_action_count[PM_DEVICE_ACTION_SUSPEND]);
	turn_off_count = atomic_get(&helper_action_count[PM_DEVICE_ACTION_TURN_OFF]);
	atomic_clear(&helper_domain_suspend_error);
	zassert_ok(
		z_pm_device_runtime_power_domain_action_run(helper_dev, PM_DEVICE_ACTION_TURN_OFF));
	zassert_equal(atomic_get(&helper_action_count[PM_DEVICE_ACTION_SUSPEND]), suspend_count);
	zassert_equal(atomic_get(&helper_action_count[PM_DEVICE_ACTION_TURN_OFF]), turn_off_count);
	zassert_false(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));
	zassert_false(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_RELEASING));
	zassert_equal(pm_device_runtime_usage(helper_domain), 0);

	zassert_ok(
		z_pm_device_runtime_power_domain_action_run(helper_dev, PM_DEVICE_ACTION_TURN_ON));
	zassert_ok(pm_device_runtime_get(helper_dev));
	atomic_set(&helper_action_error[PM_DEVICE_ACTION_SUSPEND], -EFAULT);
	atomic_set(&helper_domain_suspend_error, -EIO);
	zassert_equal(
		z_pm_device_runtime_power_domain_action_run(helper_dev, PM_DEVICE_ACTION_TURN_OFF),
		-EFAULT);
	zassert_ok(pm_device_state_get(helper_dev, &state));
	zassert_equal(state, PM_DEVICE_STATE_OFF);
	zassert_equal(pm_device_runtime_usage(helper_dev), 0);
	zassert_true(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));
	zassert_false(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_RELEASING));
	zassert_equal(pm_device_runtime_usage(helper_domain), 1);
	atomic_clear(&helper_action_error[PM_DEVICE_ACTION_SUSPEND]);
	atomic_clear(&helper_domain_suspend_error);
	zassert_ok(
		z_pm_device_runtime_power_domain_action_run(helper_dev, PM_DEVICE_ACTION_TURN_OFF));
	zassert_false(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));
	zassert_equal(pm_device_runtime_usage(helper_domain), 0);
	zassert_ok(
		z_pm_device_runtime_power_domain_action_run(helper_dev, PM_DEVICE_ACTION_TURN_ON));
	zassert_ok(pm_device_runtime_disable(helper_dev));
	zassert_ok(pm_device_runtime_disable(helper_domain));
}

ZTEST(power_domain_1cpu, test_power_domain_async_off_releasing_failure)
{
	const struct device *helper_domain = DEVICE_DT_GET(TEST_DOMAIN_HELPER);
	const struct device *helper_dev = DEVICE_DT_GET(TEST_DEV_HELPER);
	atomic_val_t suspend_count;
	atomic_val_t turn_off_count;
	enum pm_device_state state;
	k_tid_t get_tid;
	int get_done_ret;

	if (IS_ENABLED(CONFIG_TEST_PM_DEVICE_ISR_SAFE)) {
		ztest_test_skip();
		return;
	}

	helper_actions_reset();
	atomic_clear(&helper_domain_suspend_error);
	atomic_set(&helper_domain_suspend_error_after_action, -EIO);
	atomic_clear(&helper_domain_block_suspend);
	atomic_set(&helper_domain_block_after_action, 1);
	atomic_clear(&helper_direct_turn_off_claimed);
	atomic_clear(&helper_direct_turn_off_releasing);
	k_sem_reset(&helper_domain_suspend_entered);
	k_sem_reset(&helper_domain_suspend_continue);
	k_sem_reset(&helper_get_started);
	k_sem_reset(&helper_get_done);

	pm_device_init_suspended(helper_domain);
	pm_device_init_suspended(helper_dev);
	zassert_ok(pm_device_runtime_enable(helper_domain));
	zassert_ok(pm_device_runtime_enable(helper_dev));
	zassert_ok(pm_device_runtime_get(helper_dev));
	zassert_ok(pm_device_runtime_put_async(helper_dev, K_NO_WAIT));
	zassert_ok(k_sem_take(&helper_domain_suspend_entered, K_MSEC(100)));
	zassert_ok(pm_device_state_get(helper_dev, &state));
	zassert_equal(state, PM_DEVICE_STATE_OFF);
	zassert_true(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));
	zassert_true(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_RELEASING));

	get_tid = k_thread_create(&helper_get_thread, helper_get_stack,
				  K_THREAD_STACK_SIZEOF(helper_get_stack), helper_runtime_get,
				  (void *)helper_dev, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	zassert_ok(k_sem_take(&helper_get_started, K_MSEC(100)));
	get_done_ret = k_sem_take(&helper_get_done, K_MSEC(10));
	if (get_done_ret != 0) {
		k_sem_give(&helper_domain_suspend_continue);
	}
	zassert_ok(get_done_ret);
	zassert_ok(k_thread_join(get_tid, K_MSEC(100)));
	zassert_equal(helper_get_ret, -EAGAIN);
	zassert_equal(pm_device_runtime_usage(helper_dev), 0);
	zassert_equal(pm_device_runtime_usage(helper_domain), 0);

	k_sem_give(&helper_domain_suspend_continue);
	zassert_true(
		WAIT_FOR(!atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_RELEASING),
			 100000, k_msleep(1)));
	zassert_true(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));
	zassert_equal(pm_device_runtime_usage(helper_domain), 1);
	zassert_equal(atomic_get(&helper_direct_turn_off_claimed), 1);
	zassert_equal(atomic_get(&helper_direct_turn_off_releasing), 1);

	suspend_count = atomic_get(&helper_action_count[PM_DEVICE_ACTION_SUSPEND]);
	turn_off_count = atomic_get(&helper_action_count[PM_DEVICE_ACTION_TURN_OFF]);
	atomic_clear(&helper_domain_suspend_error_after_action);
	atomic_clear(&helper_domain_block_after_action);
	zassert_ok(
		z_pm_device_runtime_power_domain_action_run(helper_dev, PM_DEVICE_ACTION_TURN_OFF));
	zassert_equal(atomic_get(&helper_action_count[PM_DEVICE_ACTION_SUSPEND]), suspend_count);
	zassert_equal(atomic_get(&helper_action_count[PM_DEVICE_ACTION_TURN_OFF]), turn_off_count);
	zassert_false(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));
	zassert_equal(pm_device_runtime_usage(helper_domain), 0);

	zassert_ok(
		z_pm_device_runtime_power_domain_action_run(helper_dev, PM_DEVICE_ACTION_TURN_ON));
	zassert_ok(pm_device_runtime_disable(helper_dev));
	zassert_ok(pm_device_runtime_disable(helper_domain));
}

ZTEST(power_domain_1cpu, test_power_domain_async_claim_handoff)
{
	const struct device *helper_domain = DEVICE_DT_GET(TEST_DOMAIN_HELPER);
	const struct device *helper_dev = DEVICE_DT_GET(TEST_DEV_HELPER);
	enum pm_device_state state;
	k_tid_t get_tid;

	if (IS_ENABLED(CONFIG_TEST_PM_DEVICE_ISR_SAFE)) {
		ztest_test_skip();
		return;
	}

	helper_actions_reset();
	atomic_clear(&helper_domain_suspend_error);
	atomic_set(&helper_domain_block_suspend, 1);
	atomic_clear(&helper_direct_turn_off_claimed);
	atomic_clear(&helper_direct_turn_off_releasing);
	k_sem_reset(&helper_domain_suspend_entered);
	k_sem_reset(&helper_domain_suspend_continue);
	k_sem_reset(&helper_get_started);
	k_sem_reset(&helper_get_done);

	pm_device_init_suspended(helper_domain);
	pm_device_init_suspended(helper_dev);
	zassert_ok(pm_device_runtime_enable(helper_domain));
	zassert_ok(pm_device_runtime_enable(helper_dev));
	zassert_ok(pm_device_runtime_get(helper_dev));
	zassert_equal(pm_device_runtime_usage(helper_domain), 1);

	zassert_ok(pm_device_runtime_put_async(helper_dev, K_NO_WAIT));
	zassert_ok(k_sem_take(&helper_domain_suspend_entered, K_MSEC(100)));
	zassert_ok(pm_device_state_get(helper_dev, &state));
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);
	zassert_equal(pm_device_runtime_usage(helper_dev), 0);
	zassert_true(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));
	zassert_true(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_RELEASING));
	zassert_equal(pm_device_runtime_usage(helper_domain), 0);

	get_tid = k_thread_create(&helper_get_thread, helper_get_stack,
				  K_THREAD_STACK_SIZEOF(helper_get_stack), helper_runtime_get,
				  (void *)helper_dev, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	zassert_ok(k_sem_take(&helper_get_started, K_MSEC(100)));
	zassert_equal(k_sem_take(&helper_get_done, K_MSEC(10)), -EAGAIN);
	zassert_equal(pm_device_runtime_usage(helper_dev), 0);
	zassert_equal(pm_device_runtime_usage(helper_domain), 0);
	zassert_true(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));
	zassert_true(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_RELEASING));

	k_sem_give(&helper_domain_suspend_continue);
	zassert_ok(k_sem_take(&helper_get_done, K_MSEC(100)));
	zassert_ok(k_thread_join(get_tid, K_MSEC(100)));
	zassert_equal(helper_get_ret, -EAGAIN);
	zassert_equal(atomic_get(&helper_direct_turn_off_claimed), 1);
	zassert_equal(atomic_get(&helper_direct_turn_off_releasing), 1);
	zassert_ok(pm_device_state_get(helper_dev, &state));
	zassert_equal(state, PM_DEVICE_STATE_OFF);
	zassert_equal(pm_device_runtime_usage(helper_dev), 0);
	zassert_equal(pm_device_runtime_usage(helper_domain), 0);
	zassert_false(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));
	zassert_false(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_RELEASING));
	zassert_equal(atomic_get(&helper_action_count[PM_DEVICE_ACTION_SUSPEND]), 1);
	zassert_equal(atomic_get(&helper_action_count[PM_DEVICE_ACTION_TURN_OFF]), 1);

	atomic_clear(&helper_domain_block_suspend);
	zassert_ok(pm_device_runtime_disable(helper_dev));
	zassert_ok(pm_device_runtime_disable(helper_domain));
}

ZTEST(power_domain_1cpu, test_power_domain_async_claim_handoff_failure)
{
	const struct device *helper_domain = DEVICE_DT_GET(TEST_DOMAIN_HELPER);
	const struct device *helper_dev = DEVICE_DT_GET(TEST_DEV_HELPER);
	enum pm_device_state state;
	k_tid_t get_tid;

	if (IS_ENABLED(CONFIG_TEST_PM_DEVICE_ISR_SAFE)) {
		ztest_test_skip();
		return;
	}

	helper_actions_reset();
	atomic_set(&helper_domain_suspend_error, -EIO);
	atomic_set(&helper_domain_block_suspend, 1);
	k_sem_reset(&helper_domain_suspend_entered);
	k_sem_reset(&helper_domain_suspend_continue);
	k_sem_reset(&helper_get_started);
	k_sem_reset(&helper_get_done);

	pm_device_init_suspended(helper_domain);
	pm_device_init_suspended(helper_dev);
	zassert_ok(pm_device_runtime_enable(helper_domain));
	zassert_ok(pm_device_runtime_enable(helper_dev));
	zassert_ok(pm_device_runtime_get(helper_dev));
	zassert_equal(pm_device_runtime_usage(helper_domain), 1);

	zassert_ok(pm_device_runtime_put_async(helper_dev, K_NO_WAIT));
	zassert_ok(k_sem_take(&helper_domain_suspend_entered, K_MSEC(100)));
	zassert_true(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));
	zassert_true(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_RELEASING));
	zassert_equal(pm_device_runtime_usage(helper_dev), 0);
	zassert_equal(pm_device_runtime_usage(helper_domain), 0);

	get_tid = k_thread_create(&helper_get_thread, helper_get_stack,
				  K_THREAD_STACK_SIZEOF(helper_get_stack), helper_runtime_get,
				  (void *)helper_dev, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	zassert_ok(k_sem_take(&helper_get_started, K_MSEC(100)));
	zassert_equal(k_sem_take(&helper_get_done, K_MSEC(10)), -EAGAIN);
	zassert_equal(pm_device_runtime_usage(helper_dev), 0);
	zassert_equal(pm_device_runtime_usage(helper_domain), 0);

	k_sem_give(&helper_domain_suspend_continue);
	zassert_ok(k_sem_take(&helper_get_done, K_MSEC(100)));
	zassert_ok(k_thread_join(get_tid, K_MSEC(100)));
	zassert_ok(helper_get_ret);
	zassert_ok(pm_device_state_get(helper_dev, &state));
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE);
	zassert_equal(pm_device_runtime_usage(helper_dev), 1);
	zassert_equal(pm_device_runtime_usage(helper_domain), 1);
	zassert_true(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));
	zassert_false(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_RELEASING));
	zassert_equal(atomic_get(&helper_action_count[PM_DEVICE_ACTION_SUSPEND]), 1);
	zassert_equal(atomic_get(&helper_action_count[PM_DEVICE_ACTION_RESUME]), 2);
	zassert_equal(atomic_get(&helper_action_count[PM_DEVICE_ACTION_TURN_OFF]), 0);

	atomic_clear(&helper_domain_suspend_error);
	atomic_clear(&helper_domain_block_suspend);
	zassert_ok(pm_device_runtime_put(helper_dev));
	zassert_ok(pm_device_runtime_disable(helper_dev));
	zassert_ok(pm_device_runtime_disable(helper_domain));
}

ZTEST(power_domain_1cpu, test_power_domain_async_disable_handoff)
{
	const struct device *helper_domain = DEVICE_DT_GET(TEST_DOMAIN_HELPER);
	const struct device *helper_dev = DEVICE_DT_GET(TEST_DEV_HELPER);
	enum pm_device_state state;
	k_tid_t disable_tid;

	if (IS_ENABLED(CONFIG_TEST_PM_DEVICE_ISR_SAFE)) {
		ztest_test_skip();
		return;
	}

	helper_actions_reset();
	atomic_clear(&helper_domain_suspend_error);
	atomic_set(&helper_domain_block_suspend, 1);
	k_sem_reset(&helper_domain_suspend_entered);
	k_sem_reset(&helper_domain_suspend_continue);
	k_sem_reset(&helper_disable_started);
	k_sem_reset(&helper_disable_done);

	pm_device_init_suspended(helper_domain);
	pm_device_init_suspended(helper_dev);
	zassert_ok(pm_device_runtime_enable(helper_domain));
	zassert_ok(pm_device_runtime_enable(helper_dev));
	zassert_ok(pm_device_runtime_get(helper_dev));
	zassert_ok(pm_device_runtime_put_async(helper_dev, K_NO_WAIT));
	zassert_ok(k_sem_take(&helper_domain_suspend_entered, K_MSEC(100)));

	disable_tid =
		k_thread_create(&helper_disable_thread, helper_disable_stack,
				K_THREAD_STACK_SIZEOF(helper_disable_stack), helper_runtime_disable,
				(void *)helper_dev, NULL, NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	zassert_ok(k_sem_take(&helper_disable_started, K_MSEC(100)));
	zassert_equal(k_sem_take(&helper_disable_done, K_MSEC(10)), -EAGAIN);
	zassert_true(pm_device_runtime_is_enabled(helper_dev));
	zassert_true(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_RELEASING));

	k_sem_give(&helper_domain_suspend_continue);
	zassert_ok(k_sem_take(&helper_disable_done, K_MSEC(100)));
	zassert_ok(k_thread_join(disable_tid, K_MSEC(100)));
	zassert_ok(helper_disable_ret);
	zassert_false(pm_device_runtime_is_enabled(helper_dev));
	zassert_ok(pm_device_state_get(helper_dev, &state));
	zassert_equal(state, PM_DEVICE_STATE_OFF);
	zassert_false(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_CLAIMED));
	zassert_false(atomic_test_bit(&helper_dev->pm_base->flags, PM_DEVICE_FLAG_PD_RELEASING));
	zassert_equal(pm_device_runtime_usage(helper_domain), 0);

	atomic_clear(&helper_domain_block_suspend);
	zassert_ok(pm_device_runtime_disable(helper_domain));
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
