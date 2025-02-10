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


/* A custom policy manager is used so we can control the Power States
 * to transition to and test the issuance of TURN_ON and TURN_OFF actions
 * by the power domain driver.
 */
const struct pm_state_info *pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	const struct pm_state_info *cpu_states;

	pm_state_cpu_get_all(cpu, &cpu_states);

	if (testing_domain_on_notitication == 0) {
		/* First power state to transition is STANDBY */
		return &cpu_states[0];
	} else if (testing_domain_on_notitication == 1) {
		/* Second power state to transition is SUSPEND-TO-IDLE */
		return &cpu_states[1];
	} else if (testing_domain_on_notitication == 2) {
		/* Third power state to transition is RUNTIME-IDLE */
		return &cpu_states[2];
	}

	return NULL;
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
	/* Sleep to transition to first STATE: STANDBY */
	k_sleep(K_USEC(1));

	zassert_equal(testing_domain_on_notitication, 1);
	zassert_equal(testing_domain_off_notitication, 1);

	/* Sleep to transition to second STATE: SUSPEND-TO-IDLE */
	k_sleep(K_USEC(1));

	zassert_equal(testing_domain_on_notitication, 2);
	zassert_equal(testing_domain_off_notitication, 2);

	/* Sleep to transition to second STATE: RUNTIME-IDLE */
	k_sleep(K_USEC(1));

	/* The domain notification flags should remain the same as RUNTIME-IDLE
	 * is not listed as an ON/OFF power state in device-tree.
	 */
	zassert_equal(testing_domain_on_notitication, 2);
	zassert_equal(testing_domain_off_notitication, 2);
}

ZTEST_SUITE(power_domain_soc_state_change_1cpu, NULL, NULL, ztest_simple_1cpu_before,
	    ztest_simple_1cpu_after, NULL);
