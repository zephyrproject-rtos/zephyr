/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <device.h>
#include <pm/pm.h>
#include <pm/device_policy.h>

#define DEV_NAME DT_NODELABEL(gpio0)


static const struct device *dev_gpio;
static uint8_t sleep_count;


void pm_power_state_exit_post_ops(struct pm_state_info info)
{
	irq_unlock(0);
}

struct pm_state_info pm_policy_next_state(int32_t ticks)
{
	struct pm_state_info state_info = {PM_STATE_ACTIVE, 0, 0, 0};

	switch (sleep_count) {
	case 0:
		state_info.state = PM_STATE_STANDBY;
		break;
	case 1:
		state_info.state = PM_STATE_SUSPEND_TO_RAM;
		break;
	case 2:
		state_info.state = PM_STATE_SUSPEND_TO_DISK;
		break;
	case 3:
		__fallthrough;
	default:
		break;
	}

	sleep_count++;

	return state_info;
}

static void check_device_state(const struct device *dev)
{
	int ret;
	enum pm_device_state device_state;

	ret = pm_device_state_get(dev, &device_state);
	zassert_equal(ret, 0, "Could not query the device state");

	switch (sleep_count) {
	case 0:
	case 1:
		zassert_equal(device_state, PM_DEVICE_STATE_LOW_POWER,
			      "Device state should be PM_DEVICE_STATE_LOW_POWER");
		break;
	case 2:
		zassert_equal(device_state, PM_DEVICE_STATE_LOW_POWER,
			      "Device state should be PM_DEVICE_STATE_LOW_POWER");
		break;
	case 3:
		zassert_equal(device_state, PM_DEVICE_STATE_SUSPENDED,
			      "Device state should be PM_DEVICE_STATE_LOW_POWER");
		break;
	default:
		break;
	}
}

enum pm_device_state pm_device_policy_next_state(const struct device *dev,
					 const struct pm_state_info *state)
{
	enum pm_device_state device_state = PM_DEVICE_STATE_ACTIVE;

	/* Lets focus only in the gpio device because other devices may
	 * not support PM and we don't want it to interfer with the test.
	 */
	if (dev_gpio != dev) {
		return PM_DEVICE_STATE_ACTIVE;
	}

	switch (state->state) {
	case PM_STATE_STANDBY:
		device_state = PM_DEVICE_STATE_LOW_POWER;
		break;
	case PM_STATE_SUSPEND_TO_RAM:
		device_state = PM_DEVICE_STATE_LOW_POWER;
		break;
	case PM_STATE_SUSPEND_TO_DISK:
		device_state = PM_DEVICE_STATE_SUSPENDED;
		break;
	case PM_STATE_ACTIVE:
		/* The system is resuming here, we have to check whether or not
		 * the device is the right state.
		 */
		device_state = PM_DEVICE_STATE_ACTIVE;
		check_device_state(dev);
		break;
	default:
		zassert_true(0, "Unexpected system state");
		device_state = PM_DEVICE_STATE_ACTIVE;
		break;
	}

	return device_state;
}

void test_pm_device_policy(void)
{
	dev_gpio = DEVICE_DT_GET(DEV_NAME);

	/*
	 * Trigger system PM. The policy manager will return the
	 * following states:
	 *
	 *  - PM_STATE_STANDBY
	 *  - PM_STATE_SUSPEND_TO_RAM
	 *  - PM_STATE_SUSPEND_TO_DISK
	 *  - PM_STATE_ACTIVE
	 *
	 * The device policy will then select the following states:
	 *
	 *  - PM_DEVICE_STATE_LOW_POWER
	 *  - PM_DEVICE_STATE_LOW_POWER
	 *  - PM_DEVICE_STATE_SUSPENDED
	 *  - PM_DEVICE_STATE_ACTIVE
	 *
	 * In each iteration we will be able to check the current state and
	 * ensure that the policy is being honored.
	 *
	 * as the native posix implementation does not properly sleeps,
	 * the idle thread will call several times the PM subsystem. This
	 * test workaround this problem keeping track of the calls using
	 * the sleep_count variable.
	 */
	k_sleep(K_SECONDS(1));
}

void test_main(void)
{
	ztest_test_suite(pm_device_policy_test,
			 ztest_1cpu_unit_test(test_pm_device_policy)
		);
	ztest_run_test_suite(pm_device_policy_test);
}
