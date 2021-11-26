/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <pm/pm.h>
#include <pm/device.h>

#define DEV_NAME DT_NODELABEL(gpio0)


static const struct device *dev;
static uint8_t sleep_count;


void pm_power_state_set(struct pm_state_info info)
{
	ARG_UNUSED(info);

	enum pm_device_state state;

	switch (sleep_count) {
	case 1:
		/* Just  a sanity check that the system is the right state.
		 * Devices are suspended before SoC on PM_STATE_SUSPEND_TO_RAM, that is why
		 * we can check the device state here.
		 */
		zassert_equal(info.state, PM_STATE_SUSPEND_TO_RAM, "Wrong system state");

		(void)pm_device_state_get(dev, &state);
		zassert_equal(state, PM_DEVICE_STATE_SUSPENDED, "Wrong device state");

		/* Enable wakeup source. Next time the system is called
		 * to sleep, this device will still be active.
		 */
		(void)pm_device_wakeup_enable((struct device *)dev, true);
		break;
	case 2:
		zassert_equal(info.state, PM_STATE_SUSPEND_TO_RAM, "Wrong system state");

		/* Second time this function is called, the system is asked to standby
		 * and devices were suspended.
		 */
		(void)pm_device_state_get(dev, &state);
		zassert_equal(state, PM_DEVICE_STATE_ACTIVE, "Wrong device state");
		break;
	default:
		break;
	}
}

void pm_power_state_exit_post_ops(struct pm_state_info info)
{
	irq_unlock(0);
}

struct pm_state_info pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	ARG_UNUSED(cpu);

	while (sleep_count < 3) {
		sleep_count++;
		return (struct pm_state_info){PM_STATE_SUSPEND_TO_RAM, 0, 0, 0};
	}

	return (struct pm_state_info){PM_STATE_ACTIVE, 0, 0, 0};
}

void test_wakeup_device_api(void)
{
	bool ret = false;

	dev = DEVICE_DT_GET(DEV_NAME);
	zassert_not_null(dev, "Failed to get device");

	ret = pm_device_wakeup_is_capable(dev);
	zassert_true(ret, "Device marked as capable");

	ret = pm_device_wakeup_enable((struct device *)dev, true);
	zassert_true(ret, "Could not enable wakeup source");

	ret = pm_device_wakeup_is_enabled(dev);
	zassert_true(ret, "Wakeup source not enabled");

	ret = pm_device_wakeup_enable((struct device *)dev, false);
	zassert_true(ret, "Could not disable wakeup source");

	ret = pm_device_wakeup_is_enabled(dev);
	zassert_false(ret, "Wakeup source is enabled");
}

void test_wakeup_device_system_pm(void)
{
	/*
	 * Trigger system PM. The policy manager will return
	 * PM_STATE_SUSPEND_TO_RAM and then the PM subsystem will
	 * suspend all devices. As gpio is wakeup capability is not
	 * enabled, the device will be suspended.  This will be
	 * confirmed in pm_power_state_set().
	 *
	 * As the native posix implementation does not properly sleeps,
	 * the idle thread will call several times the PM subsystem. This
	 * test workaround this problem keeping track of the calls using
	 * the sleep_count variable.
	 */
	k_sleep(K_SECONDS(1));
}

void test_main(void)
{
	ztest_test_suite(wakeup_device_test,
			 ztest_1cpu_unit_test(test_wakeup_device_api),
			 ztest_1cpu_unit_test(test_wakeup_device_system_pm)
		);
	ztest_run_test_suite(wakeup_device_test);
}
