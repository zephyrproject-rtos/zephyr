/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/printk.h>
#include <zephyr/types.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <ztest.h>
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
static bool testing_device_lock;

static const struct device *device_dummy;
static struct dummy_driver_api *api;

static const struct device *device_a;
static const struct device *device_c;


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


/* Common init function for devices A,B and C */
static int device_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int device_a_pm_action(const struct device *dev,
		enum pm_device_action pm_action)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pm_action);

	return 0;
}

PM_DEVICE_DT_DEFINE(DT_INST(0, test_device_pm), device_a_pm_action);

DEVICE_DT_DEFINE(DT_INST(0, test_device_pm), device_init,
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

DEVICE_DT_DEFINE(DT_INST(1, test_device_pm), device_init,
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

DEVICE_DT_DEFINE(DT_INST(2, test_device_pm), device_init,
		PM_DEVICE_DT_GET(DT_INST(2, test_device_pm)), NULL, NULL,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		NULL);



void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);
	ARG_UNUSED(state);

	enum pm_device_state device_power_state;

	/* If testing device order this function does not need to anything */
	if (testing_device_order) {
		return;
	}

	if (testing_device_lock) {
		pm_device_state_get(device_a, &device_power_state);

		/*
		 * If the device has its state locked the device has
		 * to be ACTIVE
		 */
		zassert_true(device_power_state == PM_DEVICE_STATE_ACTIVE,
				NULL);
		return;
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
		zassert_true(device_power_state == PM_DEVICE_STATE_ACTIVE, NULL);
	} else {
		/* at this point, devices have been deactivated */
		zassert_false(device_power_state == PM_DEVICE_STATE_ACTIVE, NULL);
	}

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
	zassert_true(z_is_idle_thread_object(_current), NULL);
	zassert_true(ticks == _kernel.idle, NULL);
	zassert_false(k_can_yield(), NULL);
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
	enum pm_device_state device_power_state;

	/* enter suspend */
	zassert_true(notify_app_entry == true,
		     "Notification to enter suspend was not sent to the App");
	zassert_true(z_is_idle_thread_object(_current), NULL);
	zassert_equal(state, PM_STATE_SUSPEND_TO_IDLE, NULL);

	pm_device_state_get(device_dummy, &device_power_state);
	if (testing_device_runtime) {
		/* If device runtime is enable, the device should still be
		 * active
		 */
		zassert_true(device_power_state == PM_DEVICE_STATE_ACTIVE, NULL);
	} else {
		/* at this point, devices should not be active */
		zassert_false(device_power_state == PM_DEVICE_STATE_ACTIVE, NULL);
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
	zassert_true(z_is_idle_thread_object(_current), NULL);
	zassert_equal(state, PM_STATE_SUSPEND_TO_IDLE, NULL);

	/* at this point, devices are active again*/
	pm_device_state_get(device_dummy, &device_power_state);
	zassert_equal(device_power_state, PM_DEVICE_STATE_ACTIVE, NULL);
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
void test_power_idle(void)
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
void test_power_state_trans(void)
{
	int ret;

	pm_notifier_register(&notifier);
	enter_low_power = true;

	ret = pm_device_runtime_disable(device_dummy);
	zassert_true(ret == 0, "Failed to disable device runtime PM");

	/* give way to idle thread */
	k_sleep(SLEEP_TIMEOUT);
	zassert_true(leave_idle, NULL);

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
void test_power_state_notification(void)
{
	int ret;
	enum pm_device_state device_power_state;

	pm_notifier_register(&notifier);
	enter_low_power = true;

	ret = api->open(device_dummy);
	zassert_true(ret == 0, "Fail to open device");

	pm_device_state_get(device_dummy, &device_power_state);
	zassert_equal(device_power_state, PM_DEVICE_STATE_ACTIVE, NULL);


	/* The device should be kept active even when the system goes idle */
	testing_device_runtime = true;

	k_sleep(SLEEP_TIMEOUT);
	zassert_true(leave_idle, NULL);

	api->close(device_dummy);
	pm_device_state_get(device_dummy, &device_power_state);
	zassert_equal(device_power_state, PM_DEVICE_STATE_SUSPENDED, NULL);
	pm_notifier_unregister(&notifier);
}

void test_device_order(void)
{
	device_a = DEVICE_DT_GET(DT_INST(0, test_device_pm));
	zassert_not_null(device_a, "Failed to get device");

	device_c = DEVICE_DT_GET(DT_INST(2, test_device_pm));
	zassert_not_null(device_c, "Failed to get device");

	testing_device_order = true;
	enter_low_power = true;

	k_sleep(SLEEP_TIMEOUT);

	testing_device_order = false;
}

/**
 * @brief Test the device busy APIs.
 */
void test_busy(void)
{
	bool busy;

	busy = pm_device_is_any_busy();
	zassert_false(busy, NULL);

	pm_device_busy_set(device_dummy);

	busy = pm_device_is_any_busy();
	zassert_true(busy, NULL);

	busy = pm_device_is_busy(device_dummy);
	zassert_true(busy, NULL);

	pm_device_busy_clear(device_dummy);

	busy = pm_device_is_any_busy();
	zassert_false(busy, NULL);

	busy = pm_device_is_busy(device_dummy);
	zassert_false(busy, NULL);
}

void test_device_state_lock(void)
{
	pm_device_state_lock(device_a);
	zassert_true(pm_device_state_is_locked(device_a), NULL);

	testing_device_lock = true;
	enter_low_power = true;

	k_sleep(SLEEP_TIMEOUT);

	pm_device_state_unlock(device_a);

	testing_device_lock = false;
}

void test_main(void)
{
	device_dummy = device_get_binding(DUMMY_DRIVER_NAME);
	api = (struct dummy_driver_api *)device_dummy->api;

	ztest_test_suite(power_management_test,
			 ztest_1cpu_unit_test(test_power_idle),
			 ztest_1cpu_unit_test(test_power_state_trans),
			 ztest_1cpu_unit_test(test_device_order),
			 ztest_1cpu_unit_test(test_device_state_lock),
			 ztest_1cpu_unit_test(test_power_state_notification),
			 ztest_1cpu_unit_test(test_busy));
	ztest_run_test_suite(power_management_test);
	pm_notifier_unregister(&notifier);
}
