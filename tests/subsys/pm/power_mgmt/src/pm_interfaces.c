/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <zephyr/types.h>
#include <ztest.h>
#include <ksched.h>
#include <kernel.h>
#include <pm/pm.h>
#include <pm/device_runtime.h>
#include "dummy_driver.h"

static bool enter_low_power;
static bool enter_deep;
static int deep_entered;
static int deep_left;
static bool enter_unknown;
static bool pm_state_force;

const struct device *dev;
struct dummy_driver_api *api;

/* SOC specific power state set function
 * implement here for testing purpose
 */
void pm_power_state_set(struct pm_state_info info)
{
	zassert_not_equal(info.state, PM_STATE_ACTIVE,
			  "Should not be force into active state");
	pm_state_force = true;
}

/* A pm policy for testing purpose */
#define PM_STATE_UNKNOWN 0xFF
struct pm_state_info pm_policy_next_state(int ticks)
{
	struct pm_state_info info;

	memset(&info, 0, sizeof(struct pm_state_info));
	if (enter_low_power) {
		enter_low_power = false;
		if (!pm_constraint_get(PM_STATE_RUNTIME_IDLE)) {
			info.state = PM_STATE_ACTIVE;
		} else {
			info.state = PM_STATE_RUNTIME_IDLE;
		}
	} else if (enter_deep) {
		enter_deep = false;
		if (!pm_constraint_get(PM_STATE_SUSPEND_TO_RAM)) {
			info.state = PM_STATE_ACTIVE;
		} else {
			info.state = PM_STATE_SUSPEND_TO_RAM;
		}
	} else if (enter_unknown) {
		enter_unknown = false;
		info.state = PM_STATE_UNKNOWN;
	} else {
		info.state = PM_STATE_ACTIVE;
	}
	return info;
}

/*
 * @brief test pm_system_suspend()
 *
 * @details pm_system_suspend() is called by idle thread, test thread should
 *          not call this interface directly, but can switch to idle thread
 *          by k_sleep()
 *
 * @ingroup power_tests
 */

#define SLEEP_TIMEOUT K_MSEC(100)
void test_pm_system_suspend(void)
{
	enter_low_power = true;
	/* give way to idle thread */
	k_sleep(SLEEP_TIMEOUT);

	enter_low_power = false;
	enter_deep = true;
	k_sleep(SLEEP_TIMEOUT);

	/* entering an unknown power state */
	enter_low_power = false;
	enter_deep = false;
	enter_unknown = true;
	k_sleep(SLEEP_TIMEOUT);

	/* a device fail to enter suspend state,
	 * that will cause system fail to sleep
	 */
	api->refuse_to_sleep(dev);

	enter_low_power = true;
	k_sleep(SLEEP_TIMEOUT);

	enter_deep = true;
	k_sleep(SLEEP_TIMEOUT);
}

/*
 * @brief test pm_power_state_force()
 *
 * @details pm_power state_force() overrides decision made by PM policy,
 *          forcing usage of given power state immediately
 *
 * @ingroup power_tests
 */
void test_pm_power_state_force(void)
{
	struct pm_state_info info;
	struct pm_state_info info_get_active;
	struct pm_state_info info_get;

	pm_state_force = false;
	info.state = PM_STATE_ACTIVE;
	pm_power_state_force(info);
	/* fail to force system enter active status */
	zassert_false(pm_state_force == true, NULL);

	info_get_active = pm_power_state_next_get();
	zassert_equal(info_get_active.state, info.state, NULL);

	info.state = PM_STATE_SUSPEND_TO_RAM;
	pm_power_state_force(info);
	/* success to force system enter suspend-to-ram status */
	zassert_true(pm_state_force == true, NULL);
	info_get = pm_power_state_next_get();
	zassert_not_equal(info_get.state, info_get_active.state, NULL);

	pm_system_resume();
}

static void notify_pm_state_entry(enum pm_state state)
{
	if (state == PM_STATE_SUSPEND_TO_RAM) {
		deep_entered++;
	}
}
static void notify_pm_state_exit(enum pm_state state)
{
	if (state == PM_STATE_SUSPEND_TO_RAM) {
		deep_left++;
	}
}

/*
 * @brief test pm_notifier_register() and pm_notifier_unregister()
 *
 * @details register and unregister a pm_notifier struct, this struct contains
 *          callbacks that are called when the target enters and exits power
 *          states.
 *
 * @ingroup power_tests
 */
void test_pm_notifier(void)
{
	int ret;

	struct pm_notifier notifier_without_callback = {
		.state_entry = NULL,
		.state_exit = NULL,
	};

	struct pm_notifier notifier = {
		.state_entry = notify_pm_state_entry,
		.state_exit = notify_pm_state_exit,
	};

	deep_entered = 0;
	deep_left = 0;

	pm_notifier_register(&notifier_without_callback);
	pm_notifier_register(&notifier);

	enter_low_power = false;
	enter_deep = true;
	k_sleep(SLEEP_TIMEOUT);
	zassert_equal(deep_entered, 1, NULL);
	zassert_equal(deep_left, 1, NULL);

	ret = pm_notifier_unregister(&notifier);
	zassert_equal(ret, 0, NULL);
	ret = pm_notifier_unregister(&notifier);
	zassert_equal(ret, -EINVAL, NULL);
	ret = pm_notifier_unregister(&notifier_without_callback);
	zassert_equal(ret, 0, NULL);
}

#define PM_DEVICE_STATE_UNKNOWN 0xFF
/*
 * @brief test pm_device_state_set()
 *
 * @details Use pm_device_state_set() to set particular device to specific state
 *
 * @ingroup power_tests
 */
void test_pm_device_state_set(void)
{
	int ret;
	enum pm_device_state state;

	pm_device_state_get(dev, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE, "wrong state");
	/* resume an active device, no error occurs */
	ret = pm_device_state_set(dev, PM_DEVICE_STATE_ACTIVE);
	zassert_equal(ret, -EALREADY, "fail to resume device");
	pm_device_state_get(dev, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE, "wrong state");
	ret = strcmp(pm_device_state_str(state), "active");
	zassert_true(ret == 0, NULL);

	/* power off
	 * no interface for powering off all devices, use pm_device_state_set()
	 * to power off dummy_device
	 */
	ret = pm_device_state_set(dev, PM_DEVICE_STATE_OFF);
	zassert_true(ret == 0, "fail to power off device");
	pm_device_state_get(dev, &state);
	zassert_equal(state, PM_DEVICE_STATE_OFF, "wrong state %d", state);
	ret = strcmp(pm_device_state_str(state), "off");
	zassert_true(ret == 0, NULL);
	/* re-power off */
	ret = pm_device_state_set(dev, PM_DEVICE_STATE_OFF);
	zassert_equal(ret, -EALREADY, "fail to power off device");
	/* suspend after power off */
	ret = pm_device_state_set(dev, PM_DEVICE_STATE_SUSPENDED);
	zassert_equal(ret, -ENOTSUP, "fail to power off device");

	/* set device in a undefined state */
	ret = pm_device_state_set(dev, PM_DEVICE_STATE_UNKNOWN);
	zassert_equal(ret, -ENOTSUP, "fail to set device state");
	ret = strcmp(pm_device_state_str(PM_DEVICE_STATE_UNKNOWN), "");
	zassert_true(ret == 0, NULL);
}

/*
 * @brief test pm_device_wakeup_enable()
 *
 * @details If a device has the capability of waking up, it can be enable or
 *          disable by pm_device_wakeup_enable()
 *
 * @ingroup power_tests
 */

void test_pm_device_wakeup_enable(void)
{
	bool ret;
	int res;
	struct device *pm_dev;

	pm_dev = (struct device *)DEVICE_DT_GET(DT_INST(0, zephyr_wakeup_dev));
	zassert_not_null(pm_dev, "Failed to get device");

	/* pm_dev is not pm enabled, request this device will get a -ENOTSUP */
	res = pm_device_get(pm_dev);
	zassert_equal(res, -ENOTSUP, NULL);
	/* enable pm_dev */
	pm_device_enable(pm_dev);
	/* re-enable pm_dev, cause pm_dev->pm->work to be scheduled */
	pm_device_enable(pm_dev);
	res = pm_device_get(pm_dev);
	zassert_equal(res, 0, NULL);

	ret = pm_device_wakeup_is_capable(pm_dev);
	zassert_true(ret, "device is not wakeup capable");

	/* dev is not wakeup capable */
	ret = pm_device_wakeup_is_capable(dev);
	zassert_false(ret, "device is wakeup capable");
	ret = pm_device_wakeup_enable((struct device *)dev, true);
	zassert_false(ret, "success to enable wakeup");
	ret = pm_device_wakeup_is_enabled(dev);
	zassert_false(ret, "device wakeup is enabled");

	ret = pm_device_wakeup_enable(pm_dev, true);
	zassert_true(ret, "fail to enable wakeup");
	ret = pm_device_wakeup_is_enabled(pm_dev);
	zassert_true(ret, "device wakeup is not enabled");

	ret = pm_device_wakeup_enable(pm_dev, false);
	zassert_true(ret, "fail to disenable wakeup");
	ret = pm_device_wakeup_is_enabled(pm_dev);
	zassert_false(ret, "device wakeup is not disenabled");

	/* disable pm_dev */
	pm_device_put(pm_dev);
	pm_device_disable(pm_dev);
}

/*
 * @brief test pm_constraint_set(), pm_constraint_release(),
 *        pm_constraint_get()
 *
 * @details set and unset power management constraint to influence
 *          power management policy
 *
 * @ingroup power_tests
 */
void test_pm_constraint(void)
{
	struct pm_notifier notifier = {
		.state_entry = notify_pm_state_entry,
		.state_exit = notify_pm_state_exit,
	};

	deep_entered = 0;
	deep_left = 0;
	pm_notifier_register(&notifier);

	pm_constraint_set(PM_STATE_SUSPEND_TO_RAM);
	enter_low_power = false;
	enter_deep = true;
	k_sleep(SLEEP_TIMEOUT);
	zassert_equal(deep_entered, 0, NULL);
	zassert_equal(deep_left, 0, NULL);

	pm_constraint_release(PM_STATE_SUSPEND_TO_RAM);
	enter_low_power = false;
	enter_deep = true;
	k_sleep(SLEEP_TIMEOUT);
	zassert_equal(deep_entered, 1, NULL);
	zassert_equal(deep_left, 1, NULL);
	pm_notifier_unregister(&notifier);
}

void test_setup(void)
{
	dev = device_get_binding(DUMMY_NAME);
	api = (struct dummy_driver_api *)dev->api;
	api->open(dev);
}

void test_teardown(void)
{
	api->close(dev);
}

void test_main(void)
{
	ztest_test_suite(power_management_test,
			ztest_unit_test_setup_teardown(test_pm_system_suspend,
							test_setup,
							test_teardown),
			ztest_unit_test(test_pm_power_state_force),
			ztest_unit_test(test_pm_notifier),
			ztest_unit_test(test_pm_constraint),
			ztest_unit_test_setup_teardown(test_pm_device_state_set,
							test_setup,
							test_teardown),
			ztest_unit_test_setup_teardown(
				test_pm_device_wakeup_enable,
				test_setup,
				test_teardown)
			);
	ztest_run_test_suite(power_management_test);
}
