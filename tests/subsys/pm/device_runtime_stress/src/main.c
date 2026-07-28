/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/ztest.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include "emulated_pm_device.h"

#define STRESS_ITERATIONS 1024
#define GET_JITTER_PHASES 24

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

		/* The asynchronous suspend completes on the system workqueue, which
		 * on SMP may still be running when the timer handler signals
		 * completion, so poll for the suspended state with a bounded wait.
		 */
		zassert_true(WAIT_FOR((pm_device_state_get(stress_dev, &state) == 0) &&
				      (state == PM_DEVICE_STATE_SUSPENDED),
				      100000, k_msleep(1)),
			     "iter %d", i);
	}
}

ZTEST(device_runtime_stress, test_pm_runtime_timer_emulated_irq_get)
{
	enum pm_device_state state;
	int wait_us = k_ticks_to_us_ceil32(1);
	bool seen_blocked = false;
	bool hammer = false;
	int hammer_delay = 0;
	int delay;

	/* A relative K_TICKS(1) timeout armed right after a tick boundary expires
	 * on the second boundary (one full tick period is guaranteed), so sweep
	 * the put() position around two ticks worth of microseconds.
	 */
	int expiry_us = 2 * wait_us;

	if (expiry_us < STRESS_ITERATIONS / 2) {
		delay = 1;
	} else {
		delay = expiry_us - STRESS_ITERATIONS / 2;
	}

	for (int i = 0; i < STRESS_ITERATIONS; i++) {
		int ret;
		int isr_ret;

		/* Align the iteration start to a tick boundary so that the expiry
		 * position of the K_TICKS(1) timer relative to the put() below
		 * depends only on the swept delay.
		 */
		k_sleep(K_TICKS(1));

		zassert_equal(pm_device_runtime_usage(stress_dev), 0, "iter %d", i);

		ret = pm_device_runtime_get(stress_dev);
		zassert_ok(ret, "iter %d get", i);

		(void)pm_device_state_get(stress_dev, &state);
		zassert_equal(state, PM_DEVICE_STATE_ACTIVE, "iter %d", i);

		/* Arm a timer emulating a device interrupt that calls get(). */
		emulated_pm_stress_isr_get_submit(stress_dev);

		/* Sweep the put() position across the timer expiry in 1 usec steps.
		 * While the expiry lands inside the put path the handler get()
		 * reports -EWOULDBLOCK; the racy usage counter update sits right at
		 * the trailing edge of that window (expiry at the put entry). Once
		 * the sweep steps past the window, hammer its trailing edge and add
		 * a sub-usec jitter so that consecutive iterations sample different
		 * instruction-level interleavings of the last-user put path.
		 */
		if (hammer) {
			k_busy_wait(hammer_delay - 2 + (i % 3));
			for (volatile int j = (i / 3) % GET_JITTER_PHASES; j > 0; j--) {
			}
		} else {
			k_busy_wait(delay);
			delay++;
		}

		ret = pm_device_runtime_put(stress_dev);
		zassert_ok(ret, "iter %d put", i);

		isr_ret = emulated_pm_stress_isr_get_result(stress_dev);

		if (!hammer) {
			if (isr_ret == -EWOULDBLOCK) {
				seen_blocked = true;
			} else if (seen_blocked) {
				hammer = true;
				hammer_delay = delay - 1;
			} else {
				/* Still sweeping toward the timer expiry. */
			}
		}

		if (isr_ret == 0) {
			/* The timer handler acquired a runtime PM reference, so the
			 * device must stay active with its usage counted until the
			 * matching put().
			 */
			zassert_equal(pm_device_runtime_usage(stress_dev), 1,
				      "iter %d ISR get reference lost", i);
			(void)pm_device_state_get(stress_dev, &state);
			zassert_equal(state, PM_DEVICE_STATE_ACTIVE, "iter %d", i);

			ret = pm_device_runtime_put(stress_dev);
			zassert_ok(ret, "iter %d balancing put", i);
		} else {
			/* The only allowed failure is -EWOULDBLOCK, when the put
			 * already owns the device lock.
			 */
			zassert_equal(isr_ret, -EWOULDBLOCK, "iter %d ISR get: %d", i,
				      isr_ret);
		}

		zassert_equal(pm_device_runtime_usage(stress_dev), 0, "iter %d", i);
		(void)pm_device_state_get(stress_dev, &state);
		zassert_equal(state, PM_DEVICE_STATE_SUSPENDED, "iter %d", i);
	}
}

ZTEST_SUITE(device_runtime_stress, NULL, device_runtime_stress_setup, NULL, NULL,
	    device_runtime_stress_suite_teardown);
