/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>

static const struct device *const dev =
	DEVICE_DT_GET(DT_NODELABEL(gpio0));
/* Same, but names standby in zephyr,wakeup-disabling-power-states. */
static const struct device *const dev_no_wake =
	DEVICE_DT_GET(DT_NODELABEL(gpio1));
/* Same, but names suspend-to-ram in zephyr,disabling-power-states. */
static const struct device *const dev_unpowered =
	DEVICE_DT_GET(DT_NODELABEL(gpio2));
/* Same, but names a state in each of the two properties. */
static const struct device *const dev_both =
	DEVICE_DT_GET(DT_NODELABEL(gpio3));
static uint8_t sleep_count;


void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	enum pm_device_state dev_state;

	switch (sleep_count) {
	case 1:
		/* Just  a coherence check that the system is the right state.
		 * Devices are suspended before SoC on PM_STATE_SUSPEND_TO_RAM, that is why
		 * we can check the device state here.
		 */
		zassert_equal(state, PM_STATE_SUSPEND_TO_RAM, "Wrong system state");

		(void)pm_device_state_get(dev, &dev_state);
		zassert_equal(dev_state, PM_DEVICE_STATE_SUSPENDED, "Wrong device state");

		/* Enable wakeup source. Next time the system is called
		 * to sleep, this device will still be active.
		 */
		(void)pm_device_wakeup_enable(dev, true);
		break;
	case 2:
		zassert_equal(state, PM_STATE_SUSPEND_TO_RAM, "Wrong system state");

		/* Second time this function is called, the system is asked to standby
		 * and devices were suspended.
		 */
		(void)pm_device_state_get(dev, &dev_state);
		zassert_equal(dev_state, PM_DEVICE_STATE_ACTIVE, "Wrong device state");
		break;
	default:
		break;
	}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);
}

const struct pm_state_info *pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	const struct pm_state_info *cpu_states;

	zassert_true(pm_state_cpu_get_all(cpu, &cpu_states) == 1,
		     "There is no power state defined");

	while (sleep_count < 3) {
		sleep_count++;
		return &cpu_states[0];
	}

	return NULL;
}

ZTEST(wakeup_device_1cpu, test_wakeup_device_api)
{
	bool ret = false;

	zassert_true(device_is_ready(dev), "Device not ready");

	ret = pm_device_wakeup_is_capable(dev);
	zassert_true(ret, "Device not marked as capable");

	ret = pm_device_wakeup_enable(dev, true);
	zassert_true(ret, "Could not enable wakeup source");

	ret = pm_device_wakeup_is_enabled(dev);
	zassert_true(ret, "Wakeup source not enabled");

	ret = pm_device_wakeup_enable(dev, false);
	zassert_true(ret, "Could not disable wakeup source");

	ret = pm_device_wakeup_is_enabled(dev);
	zassert_false(ret, "Wakeup source is enabled");
}

ZTEST(wakeup_device_1cpu, test_wakeup_device_api_per_state)
{
	zassert_true(device_is_ready(dev_no_wake), "Device not ready");
	zassert_true(device_is_ready(dev_unpowered), "Device not ready");

	/* Names no state it cannot wake from: capable from every state, which
	 * is what pm_device_wakeup_is_capable() has always reported.
	 */
	zassert_true(pm_device_wakeup_is_capable(dev), "Device not marked as capable");
	zassert_true(pm_device_wakeup_is_capable_from_state(dev, PM_STATE_SUSPEND_TO_RAM, 0),
		     "Device naming no state must be capable from every state");
	zassert_true(pm_device_wakeup_is_capable_from_state(dev, PM_STATE_STANDBY, 0),
		     "Device naming no state must be capable from every state");

	/* zephyr,wakeup-disabling-power-states = <&state1>, i.e. not standby. */
	zassert_true(pm_device_wakeup_is_capable(dev_no_wake), "Device not marked as capable");
	zassert_false(pm_device_wakeup_is_capable_from_state(dev_no_wake, PM_STATE_STANDBY, 0),
		      "Device must not be capable from a state it named");
	zassert_true(pm_device_wakeup_is_capable_from_state(dev_no_wake,
							   PM_STATE_SUSPEND_TO_RAM, 0),
		     "Device must be capable from a state it did not name");

	/* zephyr,disabling-power-states = <&state0>: unpowered in
	 * suspend-to-ram, so not capable from it either, without the node
	 * having to name the state twice.
	 */
	zassert_true(pm_device_wakeup_is_capable(dev_unpowered), "Device not marked as capable");
	zassert_false(pm_device_wakeup_is_capable_from_state(dev_unpowered,
							    PM_STATE_SUSPEND_TO_RAM, 0),
		      "A state that removes the device power must not be wake capable");
	zassert_true(pm_device_wakeup_is_capable_from_state(dev_unpowered, PM_STATE_STANDBY, 0),
		     "Device must be capable from a state that keeps its power");

	/* Names a state in each property: both answers come back false, and a
	 * state named by neither is unaffected.
	 */
	zassert_true(device_is_ready(dev_both), "Device not ready");
	zassert_false(pm_device_wakeup_is_capable_from_state(dev_both, PM_STATE_STANDBY, 0),
		      "Device must not be capable from the state it named as no-wakeup");
	zassert_false(pm_device_wakeup_is_capable_from_state(dev_both,
							    PM_STATE_SUSPEND_TO_RAM, 0),
		      "Device must not be capable from the state that removes its power");
	zassert_true(pm_device_wakeup_is_capable_from_state(dev_both, PM_STATE_SOFT_OFF, 0),
		     "Device must be capable from a state neither property names");
}

ZTEST(wakeup_device_1cpu, test_wakeup_device_system_pm)
{
	/*
	 * Trigger system PM. The policy manager will return
	 * PM_STATE_SUSPEND_TO_RAM and then the PM subsystem will
	 * suspend all devices. As gpio is wakeup capability is not
	 * enabled, the device will be suspended.  This will be
	 * confirmed in pm_state_set().
	 *
	 * As the native posix implementation does not properly sleeps,
	 * the idle thread will call several times the PM subsystem. This
	 * test workaround this problem keeping track of the calls using
	 * the sleep_count variable.
	 */
	k_sleep(K_SECONDS(1));
}

ZTEST_SUITE(wakeup_device_1cpu, NULL, NULL, ztest_simple_1cpu_before,
			ztest_simple_1cpu_after, NULL);
