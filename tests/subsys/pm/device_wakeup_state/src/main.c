/*
 * Copyright (c) 2021 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <device.h>
#include <pm/pm.h>

void test_wakeup_device_state(void)
{
	const struct device *dev;

	dev = DEVICE_DT_GET(DT_NODELABEL(dev_default));
	zassert_true(dev->pm->state == PM_DEVICE_STATE_ACTIVE, "Wrong default state");

	dev = DEVICE_DT_GET(DT_NODELABEL(dev_active));
	zassert_true(dev->pm->state == PM_DEVICE_STATE_ACTIVE, "Wrong active state");

	dev = DEVICE_DT_GET(DT_NODELABEL(dev_low_power));
	zassert_true(dev->pm->state == PM_DEVICE_STATE_LOW_POWER, "Wrong low power state");

	dev = DEVICE_DT_GET(DT_NODELABEL(dev_suspended));
	zassert_true(dev->pm->state == PM_DEVICE_STATE_SUSPENDED, "Wrong suspended state");

	dev = DEVICE_DT_GET(DT_NODELABEL(dev_off));
	zassert_true(dev->pm->state == PM_DEVICE_STATE_OFF, "Wrong off state");
}

void test_main(void)
{
	ztest_test_suite(wakeup_state_device_test,
			 ztest_unit_test(test_wakeup_device_state)
		);
	ztest_run_test_suite(wakeup_state_device_test);
}
