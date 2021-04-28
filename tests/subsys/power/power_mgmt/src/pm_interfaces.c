/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/printk.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <ztest.h>
#include <ksched.h>
#include <kernel.h>
#include <power/power.h>
#include "pm_policy.h"
#include "dummy_driver.h"

#define SLEEP_MSEC 100
#define SLEEP_TIMEOUT K_MSEC(SLEEP_MSEC)
static bool enter_low_power;
static bool enter_deep;
static const struct device *dev;
static struct dummy_driver_api *api;

const char * const z_pm_core_devices[] = {"dummy_driver", NULL};

/* Our PM policy handler */
struct pm_state_info pm_policy_next_state(int ticks)
{
	struct pm_state_info info;
	/* make sure this is idle thread */
	zassert_true(z_is_idle_thread_object(_current), NULL);
	zassert_true(ticks == _kernel.idle, NULL);

	if (enter_low_power) {
		enter_low_power = false;
		info.state = PM_STATE_RUNTIME_IDLE;
	} else if (enter_deep) {
		enter_deep = false;
		info.state = PM_STATE_SUSPEND_TO_RAM;
	} else {
		info.state = PM_STATE_ACTIVE;
	}
	return info;
}

/*
 * @brief test power state transition
 *
 * @details if some device can not to be suspended, the suspend process
 * will abort.
 *
 * @ingroup power_tests
 */
void test_device_cannot_suspend(void)
{
	int count, count1;

	struct pm_state_info info;
	/* system enter low power state
	 * the dummy device enter low power state once
	 */
	count = api->low_power_times(dev);
	enter_low_power = true;
	/* give way to idle thread */
	k_sleep(SLEEP_TIMEOUT);
	count1 = api->low_power_times(dev);
	zassert_true((count1 - count) == 1, NULL);
	pm_dump_debug_info();

	api->busy(dev);
	/* with a device busy, system cannot enter low power state*/
	enter_low_power = true;
	k_sleep(SLEEP_TIMEOUT);
	count = api->low_power_times(dev);
	zassert_equal(count, count1, NULL);
	pm_dump_debug_info();

	enter_low_power = false;
	enter_deep = true;
	k_sleep(SLEEP_TIMEOUT);
	count = api->low_power_times(dev);
	zassert_equal(count, count1, NULL);

	/* even with pm_power_state_force() */
	info.state = PM_STATE_SUSPEND_TO_RAM;
	pm_power_state_force(info);
	count = api->low_power_times(dev);
	zassert_equal(count, count1, NULL);

	/* since system never enter low power state, no harm done to
	 * call the resume function
	 */
	pm_system_resume();
}

void test_setup(void)
{

	dev = device_get_binding(DUMMY_DRIVER_NAME);
	api = (struct dummy_driver_api *)dev->api;
	api->open(dev);
}

void test_teardown(void)
{
	api->close(dev);
}

void test_suspend_device(void)
{
	int ret;
	uint32_t device_power_state;

	api->busy(dev);
	/* can't suspend a busy device */
	ret = pm_suspend_devices();
	zassert_equal(ret, -EBUSY, NULL);
	device_get_power_state(dev, &device_power_state);
	zassert_not_equal(device_power_state, DEVICE_PM_SUSPEND_STATE, NULL);
	/* then force suspend devices */
	ret = pm_force_suspend_devices();
	zassert_equal(ret, 0, NULL);
	device_get_power_state(dev, &device_power_state);
	zassert_equal(device_power_state, DEVICE_PM_FORCE_SUSPEND_STATE, NULL);
	pm_resume_devices();
}

#define DEVICE_PM_UNKNOWN_STATE 0xFF
void test_device_pm_state_str(void)
{
	int ret;

	ret = strcmp(device_pm_state_str(DEVICE_PM_ACTIVE_STATE), "active");
	zassert_true(ret == 0, NULL);

	ret = strcmp(device_pm_state_str(DEVICE_PM_LOW_POWER_STATE),
			"low power");
	zassert_true(ret == 0, NULL);
	ret = strcmp(device_pm_state_str(DEVICE_PM_SUSPEND_STATE), "suspend");
	zassert_true(ret == 0, NULL);
	ret = strcmp(device_pm_state_str(DEVICE_PM_FORCE_SUSPEND_STATE),
			"force suspend");
	zassert_true(ret == 0, NULL);
	ret = strcmp(device_pm_state_str(DEVICE_PM_OFF_STATE), "off");
	zassert_true(ret == 0, NULL);

	ret = strcmp(device_pm_state_str(DEVICE_PM_UNKNOWN_STATE), "");
	zassert_true(ret == 0, NULL);
}

void test_device_pm_disable(void)
{
	api->pm_disable(dev);
	zassert_equal(dev->pm->enable, false, NULL);
}

void test_main(void)
{
	ztest_test_suite(power_management_test,
			 ztest_unit_test_setup_teardown(test_suspend_device,
							test_setup,
							test_teardown),
			 ztest_unit_test_setup_teardown(test_device_cannot_suspend,
							test_setup,
							test_teardown),
			 ztest_unit_test(test_device_pm_disable),
			 ztest_unit_test(test_device_pm_state_str));
	ztest_run_test_suite(power_management_test);
}
