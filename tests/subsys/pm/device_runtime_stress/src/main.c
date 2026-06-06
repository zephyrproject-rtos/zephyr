/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include "emulated_pm_device.h"

#define STRESS_ITERATIONS 1024

static const struct device *stress_dev;

static void *device_runtime_stress_setup(void)
{
	stress_dev = emulated_pm_stress_dev();
	zassert_not_null(stress_dev, "");

	zassert_ok(pm_device_runtime_enable(stress_dev));

	return NULL;
}

static void device_runtime_stress_suite_teardown(void *data)
{
	ARG_UNUSED(data);

	if (stress_dev != NULL) {
		(void)pm_device_runtime_disable(stress_dev);
	}
}

ZTEST(device_runtime_stress, test_pm_runtime_timer_emulated_irq)
{
	enum pm_device_state state;
	int wait_us = k_ticks_to_us_ceil32(1);
	int delay;

	if (wait_us < STRESS_ITERATIONS / 2) {
		delay = 1;
	} else {
		delay = wait_us - STRESS_ITERATIONS / 2;
	}

	for (int i = 0; i < STRESS_ITERATIONS; i++) {
		int ret;

		zassert_equal(pm_device_runtime_usage(stress_dev), 0, "iter %d", i);

		ret = pm_device_runtime_get(stress_dev);
		zassert_ok(ret, "iter %d outer get", i);

		(void)pm_device_state_get(stress_dev, &state);
		zassert_equal(state, PM_DEVICE_STATE_ACTIVE, "iter %d", i);

		ret = emulated_pm_stress_submit(stress_dev);
		zassert_ok(ret, "iter %d submit (inner get + timer arm)", i);

		/* Keep increasing delay to simulate different application code execution times.
		 * At some point, we should run into a case where PM put function is preempted
		 * by the interrupt handler.
		 */
		k_busy_wait(delay);
		delay++;

		/* Application is releasing the device and the device is self-releasing
		 * when operation is completed. Device operation is asynchronous so
		 * user code may be pre-empted. Test checks if PM runtime management
		 * is handling this correctly.
		 */
		ret = pm_device_runtime_put(stress_dev);
		zassert_ok(ret, "iter %d outer put", i);

		/* Wait for device completion without any error. */
		ret = emulated_pm_stress_wait(stress_dev);
		zassert_ok(ret, "iter %d wait (put_async from timer)", i);

		(void)pm_device_state_get(stress_dev, &state);
		zassert_equal(state, PM_DEVICE_STATE_SUSPENDED, "iter %d", i);
	}
}

ZTEST_SUITE(device_runtime_stress, NULL, device_runtime_stress_setup, NULL, NULL,
	    device_runtime_stress_suite_teardown);
