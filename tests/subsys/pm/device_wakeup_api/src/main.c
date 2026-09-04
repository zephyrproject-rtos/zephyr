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
/* Same, but wake capable only from state1, which the CPU never enters. */
static const struct device *const dev_narrow =
	DEVICE_DT_GET(DT_NODELABEL(gpio1));
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
	zassert_true(device_is_ready(dev_narrow), "Device not ready");

	/* No zephyr,wakeup-power-states: capable from every state, which is
	 * what pm_device_wakeup_is_capable() has always reported.
	 */
	zassert_true(pm_device_wakeup_is_capable(dev), "Device not marked as capable");
	zassert_true(pm_device_wakeup_is_capable_for_state(dev, PM_STATE_SUSPEND_TO_RAM, 0),
		     "Device without the property must be capable from every state");
	zassert_true(pm_device_wakeup_is_capable_for_state(dev, PM_STATE_STANDBY, 0),
		     "Device without the property must be capable from every state");

	/* zephyr,wakeup-power-states = <&state1>, i.e. standby only. */
	zassert_true(pm_device_wakeup_is_capable(dev_narrow), "Device not marked as capable");
	zassert_true(pm_device_wakeup_is_capable_for_state(dev_narrow, PM_STATE_STANDBY, 0),
		     "Device must be capable from a listed state");
	zassert_false(pm_device_wakeup_is_capable_for_state(dev_narrow,
							    PM_STATE_SUSPEND_TO_RAM, 0),
		      "Device must not be capable from an unlisted state");

	/* A device that is not wake capable at all is capable from no state. */
	(void)pm_device_wakeup_enable(dev_narrow, false);
	zassert_false(pm_device_wakeup_is_enabled(dev_narrow), "Wakeup source is enabled");
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
