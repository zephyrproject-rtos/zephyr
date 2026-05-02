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

	irq_unlock(0);
}

static int dev_pm_action(const struct device *dev, enum pm_device_action pm_action)
{
	ARG_UNUSED(dev);

	if (pm_action == PM_DEVICE_ACTION_TURN_ON) {
		testing_domain_on_notitication++;
	}

	if (pm_action == PM_DEVICE_ACTION_TURN_OFF) {
		testing_domain_off_notitication++;
	}

	return 0;
}

PM_DEVICE_DT_DEFINE(TEST_DEV, dev_pm_action);
DEVICE_DT_DEFINE(TEST_DEV, NULL, PM_DEVICE_DT_GET(TEST_DEV), NULL, NULL, POST_KERNEL, 20, NULL);

ZTEST(power_domain_soc_state_change_1cpu, test_power_domain_soc_state_change)
{
	const struct pm_state_info *cpu_states, *state;

	pm_state_cpu_get_all(_current_cpu->id, &cpu_states);

	state = &cpu_states[2];
	/* Sleep to transition to STATE: STANDBY */
	k_usleep(state->min_residency_us + state->exit_latency_us);

	zassert_equal(testing_domain_on_notitication, 1);
	zassert_equal(testing_domain_off_notitication, 1);

	state = &cpu_states[1];
	/* Sleep to transition to STATE: SUSPEND-TO-IDLE */
	k_usleep(state->min_residency_us + state->exit_latency_us);

	zassert_equal(testing_domain_on_notitication, 2);
	zassert_equal(testing_domain_off_notitication, 2);

	state = &cpu_states[0];
	/* Sleep to transition to STATE: RUNTIME-IDLE */
	k_usleep(state->min_residency_us + state->exit_latency_us);

	/* The domain notification flags should remain the same as RUNTIME-IDLE
	 * is not listed as an ON/OFF power state in device-tree.
	 */
	zassert_equal(testing_domain_on_notitication, 2);
	zassert_equal(testing_domain_off_notitication, 2);
}

ZTEST_SUITE(power_domain_soc_state_change_1cpu, NULL, NULL, ztest_simple_1cpu_before,
	    ztest_simple_1cpu_after, NULL);
