/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/pm/device.h>

#define TEST_DEV DT_NODELABEL(test_dev_soc_state_change)

static int testing_domain_on_notitication;
static int testing_domain_off_notitication;
static int testing_dev_suspend;
static int testing_dev_resume;
static bool testing_dev_fail_turn_on;

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (testing_domain_off_notitication) {
	case 1:
		zassert_equal(state, PM_STATE_STANDBY, "Wrong system state %d", state);
		break;
	case 2:
		zassert_true(
			((state == PM_STATE_SUSPEND_TO_IDLE) || (state == PM_STATE_RUNTIME_IDLE)),
			"Wrong system state %d", state);
		break;
	default:
		break;
	}

	k_cpu_idle();
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);
}

static int dev_pm_action(const struct device *dev, enum pm_device_action pm_action)
{
	ARG_UNUSED(dev);

	if (pm_action == PM_DEVICE_ACTION_TURN_ON) {
		testing_domain_on_notitication++;
		if (testing_dev_fail_turn_on) {
			return -EIO;
		}
	}

	if (pm_action == PM_DEVICE_ACTION_TURN_OFF) {
		testing_domain_off_notitication++;
	}

	if (pm_action == PM_DEVICE_ACTION_SUSPEND) {
		testing_dev_suspend++;
	}

	if (pm_action == PM_DEVICE_ACTION_RESUME) {
		testing_dev_resume++;
	}

	return 0;
}

PM_DEVICE_DT_DEFINE(TEST_DEV, dev_pm_action);
/* Same init priority as the domain: the init-entry ordinal still orders the
 * child after the domain (its power-domain dependency), while the device list
 * the suspend sweep walks has no such ordinal, so the domain is reached first
 * and force-suspends the still-active child, exercising the resume path.
 */
DEVICE_DT_DEFINE(TEST_DEV, NULL, PM_DEVICE_DT_GET(TEST_DEV), NULL, NULL, PRE_KERNEL_1,
		 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

static void check_forced_resume(int expected)
{
	const struct device *dev = DEVICE_DT_GET(TEST_DEV);
	enum pm_device_state state;

	zassert_equal(testing_dev_suspend, expected);
	zassert_equal(testing_dev_resume, expected);
	zassert_ok(pm_device_state_get(dev, &state));
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE);
}

static void check_not_resumed(int suspend_expected, int resume_expected)
{
	const struct device *dev = DEVICE_DT_GET(TEST_DEV);
	enum pm_device_state state;

	zassert_equal(testing_dev_suspend, suspend_expected);
	zassert_equal(testing_dev_resume, resume_expected);
	zassert_ok(pm_device_state_get(dev, &state));
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED);
}

ZTEST(power_domain_soc_state_change_1cpu, test_power_domain_soc_state_change)
{
	const struct pm_state_info *cpu_states, *state;

	pm_state_cpu_get_all(_current_cpu->id, &cpu_states);

	state = &cpu_states[2];
	/* Sleep to transition to STATE: STANDBY */
	k_usleep(state->min_residency_us + state->exit_latency_us);

	zassert_equal(testing_domain_on_notitication, 1);
	zassert_equal(testing_domain_off_notitication, 1);
	check_forced_resume(1);

	state = &cpu_states[1];
	/* Sleep to transition to STATE: SUSPEND-TO-IDLE */
	k_usleep(state->min_residency_us + state->exit_latency_us);

	zassert_equal(testing_domain_on_notitication, 2);
	zassert_equal(testing_domain_off_notitication, 2);
	check_forced_resume(2);

	state = &cpu_states[0];
	/* Sleep to transition to STATE: RUNTIME-IDLE */
	k_usleep(state->min_residency_us + state->exit_latency_us);

	/* The domain notification flags should remain the same as RUNTIME-IDLE
	 * is not listed as an ON/OFF power state in device-tree.
	 */
	zassert_equal(testing_domain_on_notitication, 2);
	zassert_equal(testing_domain_off_notitication, 2);
	check_forced_resume(2);

	/* A child whose TURN_ON fails is left in SUSPENDED with
	 * PM_DEVICE_FLAG_TURN_ON_FAILED set, and must not be resumed onto
	 * hardware that was never powered.
	 */
	testing_dev_fail_turn_on = true;

	state = &cpu_states[2];
	/* Sleep to transition to STATE: STANDBY */
	k_usleep(state->min_residency_us + state->exit_latency_us);

	zassert_equal(testing_domain_on_notitication, 3);
	zassert_equal(testing_domain_off_notitication, 3);
	check_not_resumed(3, 2);
}

ZTEST_SUITE(power_domain_soc_state_change_1cpu, NULL, NULL, ztest_simple_1cpu_before,
	    ztest_simple_1cpu_after, NULL);
