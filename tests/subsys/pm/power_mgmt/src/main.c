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
static bool set_pm;
static bool leave_idle;
static bool idle_entered;
static bool testing_device_runtime;
static bool testing_device_order;
static bool testing_force_state;

enum pm_state forced_state;
static const struct device *device_dummy;
static struct dummy_driver_api *api;

static const struct device *const device_a =
	DEVICE_DT_GET(DT_INST(0, test_device_pm));
static const struct device *const device_c =
	DEVICE_DT_GET(DT_INST(2, test_device_pm));

/*
 * This device does not support PM. It is used to check
 * the behavior of the PM subsystem when a device does not
 * support PM.
 */
static const struct device *const device_e =
	DEVICE_DT_GET(DT_INST(4, test_device_pm));

DEVICE_DT_DEFINE(DT_INST(4, test_device_pm), NULL,
		NULL, NULL, NULL,
		PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		NULL);

/*
 * According with the initialization level, devices A, B and C are
 * initialized in the following order A -> B -> C.
 *
 * The power management subsystem uses this order to suspend and resume
 * devices. Devices are suspended in the reverse order:
 *
 * C -> B -> A
 *
 * While resuming uses the initialization order:
 *
 * A -> B -> C
 *
 * This test checks if these order is correct checking devices A and C states
 * when suspending / resuming device B.
 */

static int device_a_pm_action(const struct device *dev,
		enum pm_device_action pm_action)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pm_action);

	return 0;
}

PM_DEVICE_DT_DEFINE(DT_INST(0, test_device_pm), device_a_pm_action);

DEVICE_DT_DEFINE(DT_INST(0, test_device_pm), NULL,
		PM_DEVICE_DT_GET(DT_INST(0, test_device_pm)), NULL, NULL,
		PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		NULL);


static int device_b_pm_action(const struct device *dev,
		enum pm_device_action pm_action)
{
	enum pm_device_state state_a;
	enum pm_device_state state_c;

	if (!testing_device_order) {
		return 0;
	}

	(void)pm_device_state_get(device_a, &state_a);
	(void)pm_device_state_get(device_c, &state_c);

	switch (pm_action) {
	case PM_DEVICE_ACTION_RESUME:
		/* Check if device C is still suspended */
		zassert_equal(state_c, PM_DEVICE_STATE_SUSPENDED,
				"Inconsistent states");
		/* Check if device A is already active */
		zassert_equal(state_a, PM_DEVICE_STATE_ACTIVE,
				"Inconsistent states");
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/* Check if device C is already suspended */
		zassert_equal(state_c, PM_DEVICE_STATE_SUSPENDED,
				"Inconsistent states");
		/* Check if device A is still active */
		zassert_equal(state_a, PM_DEVICE_STATE_ACTIVE,
				"Inconsistent states");
		break;
	default:
		break;
	}

	return 0;
}

PM_DEVICE_DT_DEFINE(DT_INST(1, test_device_pm), device_b_pm_action);

DEVICE_DT_DEFINE(DT_INST(1, test_device_pm), NULL,
		PM_DEVICE_DT_GET(DT_INST(1, test_device_pm)), NULL, NULL,
		PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		NULL);

static int device_c_pm_action(const struct device *dev,
		enum pm_device_action pm_action)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pm_action);

	return 0;
}

PM_DEVICE_DT_DEFINE(DT_INST(2, test_device_pm), device_c_pm_action);

DEVICE_DT_DEFINE(DT_INST(2, test_device_pm), NULL,
		PM_DEVICE_DT_GET(DT_INST(2, test_device_pm)), NULL, NULL,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		NULL);

static int device_init_failed(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Return error to mark device as not ready. */
	return -EIO;
}

static int device_d_pm_action(const struct device *dev,
		enum pm_device_action pm_action)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pm_action);

	zassert_unreachable("Entered PM handler for unready device");

	return 0;
}

PM_DEVICE_DT_DEFINE(DT_INST(3, test_device_pm), device_d_pm_action);

DEVICE_DT_DEFINE(DT_INST(3, test_device_pm), device_init_failed,
		PM_DEVICE_DT_GET(DT_INST(3, test_device_pm)), NULL, NULL,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		NULL);

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);
	ARG_UNUSED(state);

	enum pm_device_state device_power_state;

#ifndef CONFIG_PM_DEVICE_SYSTEM_MANAGED
	/* Devices shouldn't have changed state because system managed
	 * device power management is not enabled.
	 **/
	pm_device_state_get(device_a, &device_power_state);
	zassert_true(device_power_state == PM_DEVICE_STATE_ACTIVE,
			NULL);

	pm_device_state_get(device_c, &device_power_state);
	zassert_true(device_power_state == PM_DEVICE_STATE_ACTIVE,
			NULL);
#else
	/* If testing device order this function does not need to anything */
	if (testing_device_order) {
		return;
	}

	if (testing_force_state) {
		/* if forced to given power state was called */
		set_pm = true;
		zassert_equal(state, forced_state, NULL);
		testing_force_state = false;

		/* We have forced a state that does not trigger device power management.
		 * The device should still be active.
		 */
		pm_device_state_get(device_c, &device_power_state);
		zassert_true(device_power_state == PM_DEVICE_STATE_ACTIVE);
	}

	/* at this point, notify_pm_state_entry() implemented in
	 * this file has been called and set_pm should have been set
	 */
	zassert_true(set_pm == true,
		     "Notification to enter suspend was not sent to the App");

	/* this function is called after devices enter low power state */
	pm_device_state_get(device_dummy, &device_power_state);

	if (testing_device_runtime) {
		/* If device runtime is enable, the device should still be
		 * active
		 */
		zassert_true(device_power_state == PM_DEVICE_STATE_ACTIVE);
	} else {
		/* at this point, devices have been deactivated */
		zassert_false(device_power_state == PM_DEVICE_STATE_ACTIVE);
	}

	/* this function is called when system entering low power state, so
	 * parameter state should not be PM_STATE_ACTIVE
	 */
	zassert_false(state == PM_STATE_ACTIVE,
		      "Entering low power state with a wrong parameter");
#endif
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
	const struct pm_state_info *cpu_states;

	zassert_true(pm_state_cpu_get_all(cpu, &cpu_states) == 2,
		     "There is no power state defined");

	/* make sure this is idle thread */
	zassert_true(z_is_idle_thread_object(_current));
	zassert_true(ticks == _kernel.idle);
	zassert_false(k_can_yield());
	idle_entered = true;

	if (enter_low_power) {
		enter_low_power = false;
		notify_app_entry = true;
		return &cpu_states[0];
	}

	return NULL;
}

/* implement in application, called by idle thread */
static void notify_pm_state_entry(enum pm_state state)
{
	enum pm_device_state device_power_state;

	/* enter suspend */
	zassert_true(notify_app_entry == true,
		     "Notification to enter suspend was not sent to the App");
	zassert_true(z_is_idle_thread_object(_current));
	zassert_equal(state, PM_STATE_SUSPEND_TO_IDLE);

	pm_device_state_get(device_dummy, &device_power_state);
	if (testing_device_runtime || !IS_ENABLED(CONFIG_PM_DEVICE_SYSTEM_MANAGED)) {
		/* If device runtime is enable, the device should still be
		 * active
		 */
		zassert_true(device_power_state == PM_DEVICE_STATE_ACTIVE);
	} else {
		/* at this point, devices should not be active */
		zassert_false(device_power_state == PM_DEVICE_STATE_ACTIVE);
	}
	set_pm = true;
	notify_app_exit = true;
}

/* implement in application, called by idle thread */
static void notify_pm_state_exit(enum pm_state state)
{
	enum pm_device_state device_power_state;

	/* leave suspend */
	zassert_true(notify_app_exit == true,
		     "Notification to leave suspend was not sent to the App");
	zassert_true(z_is_idle_thread_object(_current));
	zassert_equal(state, PM_STATE_SUSPEND_TO_IDLE);

	/* at this point, devices are active again*/
	pm_device_state_get(device_dummy, &device_power_state);
	zassert_equal(device_power_state, PM_DEVICE_STATE_ACTIVE);
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

/*
 * @brief test power state transition
 *
 * @details
 *  - The system support control of power state ordering between
 *    subsystems and devices
 *  - The application can control system power state transitions in idle thread
 *    through pm_notify_pm_state_entry and pm_notify_pm_state_exit
 *
 * @see pm_notify_pm_state_entry(), pm_notify_pm_state_exit()
 *
 * @ingroup power_tests
 */
ZTEST(power_management_1cpu, test_power_state_trans)
{
	int ret;

	pm_notifier_register(&notifier);
	enter_low_power = true;

	ret = pm_device_runtime_disable(device_dummy);
	zassert_true(ret == 0, "Failed to disable device runtime PM");

	/* give way to idle thread */
	k_sleep(SLEEP_TIMEOUT);
	zassert_true(leave_idle);

	ret = pm_device_runtime_enable(device_dummy);
	zassert_true(ret == 0, "Failed to enable device runtime PM");

	pm_notifier_unregister(&notifier);
}

/*
 * @brief notification between system and device
 *
 * @details
 *  - device driver notify its power state change by pm_device_runtime_get and
 *    pm_device_runtime_put_async
 *  - system inform device system power state change through device interface
 *    pm_action_cb
 *
 * @see pm_device_runtime_get(), pm_device_runtime_put_async(),
 *      pm_device_action_run(), pm_device_state_get()
 *
 * @ingroup power_tests
 */
ZTEST(power_management_1cpu, test_power_state_notification)
{
	int ret;
	enum pm_device_state device_power_state;

	pm_notifier_register(&notifier);
	enter_low_power = true;

	ret = api->open(device_dummy);
	zassert_true(ret == 0, "Fail to open device");

	pm_device_state_get(device_dummy, &device_power_state);
	zassert_equal(device_power_state, PM_DEVICE_STATE_ACTIVE);


	/* The device should be kept active even when the system goes idle */
	testing_device_runtime = true;

	k_sleep(SLEEP_TIMEOUT);
	zassert_true(leave_idle);

	api->close(device_dummy);
	pm_device_state_get(device_dummy, &device_power_state);
	zassert_equal(device_power_state, PM_DEVICE_STATE_SUSPENDED);
	pm_notifier_unregister(&notifier);
	testing_device_runtime = false;
}

ZTEST(power_management_1cpu, test_device_order)
{
	zassert_true(device_is_ready(device_a), "device a not ready");
	zassert_true(device_is_ready(device_c), "device c not ready");

	testing_device_order = true;
	enter_low_power = true;

	k_sleep(SLEEP_TIMEOUT);

	testing_device_order = false;
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

ZTEST(power_management_1cpu, test_empty_states)
{
	const struct pm_state_info *cpu_states;
	uint8_t state = pm_state_cpu_get_all(1u, &cpu_states);

	zassert_equal(state, 0, NULL);
}

ZTEST(power_management_1cpu, test_force_state)
{
	const struct pm_state_info *cpu_states;

	pm_state_cpu_get_all(0, &cpu_states);
	forced_state = cpu_states[1].state;
	bool ret = pm_state_force(0, &cpu_states[1]);

	zassert_equal(ret, true, "Error in force state");

	testing_force_state = true;
	k_sleep(K_SECONDS(1U));
}

ZTEST(power_management_1cpu, test_device_without_pm)
{
	pm_device_busy_set(device_e);

	/* Since this device does not support PM, it should not be set busy */
	zassert_false(pm_device_is_busy(device_e));

	/* No device should be busy */
	zassert_false(pm_device_is_any_busy());

	/* Lets ensure that nothing happens */
	pm_device_busy_clear(device_e);

	/* Check the status. Since PM is enabled but this device does not support it.
	 * It should return -ENOSYS
	 */
	zassert_equal(pm_device_state_get(device_e, NULL), -ENOSYS);

	/* Try to forcefully change the state should also return an error */
	zassert_equal(pm_device_action_run(device_e, PM_DEVICE_ACTION_SUSPEND), -ENOSYS);

	/* Confirming the device is powered */
	zassert_true(pm_device_is_powered(device_e));

	/* Test wakeup functionality */
	zassert_false(pm_device_wakeup_enable(device_e, true));
	zassert_false(pm_device_wakeup_is_enabled(device_e));
}

void power_management_1cpu_teardown(void *data)
{
	pm_notifier_unregister(&notifier);
}

static void *power_management_1cpu_setup(void)
{
	device_dummy = device_get_binding(DUMMY_DRIVER_NAME);
	api = (struct dummy_driver_api *)device_dummy->api;
	return NULL;
}

ZTEST_SUITE(power_management_1cpu, NULL, power_management_1cpu_setup,
			ztest_simple_1cpu_before, ztest_simple_1cpu_after,
			power_management_1cpu_teardown);
