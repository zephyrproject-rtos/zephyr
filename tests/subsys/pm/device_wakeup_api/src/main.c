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
static uint8_t sleep_count;


void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	enum pm_device_state dev_state;

	switch (sleep_count) {
	case 1:
		/* Just  a sanity check that the system is the right state.
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

	irq_unlock(0);
}

const struct pm_state_info *pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	static const struct pm_state_info state = {
		.state = PM_STATE_SUSPEND_TO_RAM
	};

	ARG_UNUSED(cpu);

	while (sleep_count < 3) {
		sleep_count++;
		return &state;
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
