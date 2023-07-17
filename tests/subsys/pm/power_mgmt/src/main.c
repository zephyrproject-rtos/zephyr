/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/printk.h>
#include <zephyr/types.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/ztest.h>
#include <ksched.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include "dummy_driver.h"

#define SLEEP_MSEC 100
#define SLEEP_TIMEOUT K_MSEC(SLEEP_MSEC)

/* for checking power suspend and resume order between system and devices */
static bool enter_low_power;
static bool notify_app_entry;
static bool notify_app_exit;
static bool leave_idle;
static bool idle_entered;

static const struct device *device_dummy;

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);
	ARG_UNUSED(state);

	/* this function is called when system entering low power state, so
	 * parameter state should not be PM_STATE_ACTIVE
	 */
	zassert_false(state == PM_STATE_ACTIVE,
		      "Entering low power state with a wrong parameter");
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

	/* pm_system_suspend is entered with irq locked
	 * unlock irq before leave pm_system_suspend
	 */
	irq_unlock(0);
}

/* Our PM policy handler */
const struct pm_state_info *pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	static struct pm_state_info info;

	ARG_UNUSED(cpu);

	/* make sure this is idle thread */
	zassert_true(z_is_idle_thread_object(_current));
	zassert_true(ticks == _kernel.idle);
	zassert_false(k_can_yield());
	idle_entered = true;

	if (enter_low_power) {
		enter_low_power = false;
		notify_app_entry = true;
		info.state = PM_STATE_SUSPEND_TO_IDLE;
	} else {
		/* only test pm_policy_next_state()
		 * no PM operation done
		 */
		info.state = PM_STATE_ACTIVE;
	}
	return &info;
}

/* implement in application, called by idle thread */
static void notify_pm_state_entry(enum pm_state state)
{
	/* enter suspend */
	zassert_true(notify_app_entry == true,
		     "Notification to enter suspend was not sent to the App");
	zassert_true(z_is_idle_thread_object(_current));
	zassert_equal(state, PM_STATE_SUSPEND_TO_IDLE);

	notify_app_exit = true;
}

/* implement in application, called by idle thread */
static void notify_pm_state_exit(enum pm_state state)
{
	/* leave suspend */
	zassert_true(notify_app_exit == true,
		     "Notification to leave suspend was not sent to the App");
	zassert_true(z_is_idle_thread_object(_current));
	zassert_equal(state, PM_STATE_SUSPEND_TO_IDLE);

	leave_idle = true;

}

/*
 * @brief test power idle
 *
 * @details
 *  - The global idle routine executes when no other work is available
 *  - The idle routine provide a timeout parameter to the suspend routine
 *    indicating the amount of time guaranteed to expire before the next
 *    timeout, pm_policy_next_state() handle this parameter.
 *  - In this case, pm_policy_next_sate() return PM_STATE_ACTIVE,
 *    so there is no low power operation happen.
 *
 * @see pm_policy_next_state()
 *
 * @ingroup power_tests
 */
ZTEST(power_management_1cpu, test_power_idle)
{
	TC_PRINT("give way to idle thread\n");
	k_sleep(SLEEP_TIMEOUT);
	zassert_true(idle_entered, "Never entered idle thread");
}

static struct pm_notifier notifier = {
	.state_entry = notify_pm_state_entry,
	.state_exit = notify_pm_state_exit,
};

ZTEST(power_management_1cpu, test_power_state_notification)
{
	pm_notifier_register(&notifier);
	enter_low_power = true;

	k_sleep(SLEEP_TIMEOUT);
	zassert_true(leave_idle);

	pm_notifier_unregister(&notifier);
}

/**
 * @brief Test the device busy APIs.
 */
ZTEST(power_management_1cpu, test_busy)
{
	bool busy;

	busy = pm_device_is_any_busy();
	zassert_false(busy);

	pm_device_busy_set(device_dummy);

	busy = pm_device_is_any_busy();
	zassert_true(busy);

	busy = pm_device_is_busy(device_dummy);
	zassert_true(busy);

	pm_device_busy_clear(device_dummy);

	busy = pm_device_is_any_busy();
	zassert_false(busy);

	busy = pm_device_is_busy(device_dummy);
	zassert_false(busy);
}

void power_management_1cpu_teardown(void *data)
{
	pm_notifier_unregister(&notifier);
}

static void *power_management_1cpu_setup(void)
{
	device_dummy = device_get_binding(DUMMY_DRIVER_NAME);
	return NULL;
}

ZTEST_SUITE(power_management_1cpu, NULL, power_management_1cpu_setup,
			ztest_simple_1cpu_before, ztest_simple_1cpu_after,
			power_management_1cpu_teardown);
