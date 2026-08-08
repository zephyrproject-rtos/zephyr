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
#include <zephyr/pm/device_runtime_internal.h>

#include "emulated_pm_device.h"

#define STRESS_ITERATIONS 1024
#define GET_JITTER_PHASES 24

static const struct device *stress_dev;

enum test_runtime_operation {
	TEST_RUNTIME_GET,
	TEST_RUNTIME_PUT,
	TEST_RUNTIME_TURN_ON,
	TEST_RUNTIME_TURN_OFF,
};

struct test_runtime_operation_context {
	const struct device *dev;
	enum test_runtime_operation operation;
	int ret;
};

static atomic_t runtime_hook_enabled;
static atomic_t runtime_hook_sequence;
static atomic_t runtime_first_after_order;
static atomic_t runtime_second_after_order;
static atomic_t runtime_record_hooks;
static enum z_pm_device_runtime_test_hook runtime_hook_to_block;
static enum z_pm_device_runtime_test_hook runtime_first_after_hook;
static enum z_pm_device_runtime_test_hook runtime_second_after_hook;
K_SEM_DEFINE(runtime_hook_entered, 0, 1);
K_SEM_DEFINE(runtime_hook_continue, 0, 1);
K_SEM_DEFINE(runtime_operation_done, 0, 1);
K_THREAD_STACK_DEFINE(runtime_operation_stack, 1024);
static struct k_thread runtime_operation_thread;

void z_pm_device_runtime_test_hook(const struct device *dev,
				   enum z_pm_device_runtime_test_hook hook)
{
	if (dev != stress_dev) {
		return;
	}

	if (atomic_get(&runtime_record_hooks)) {
		if (hook == runtime_first_after_hook) {
			atomic_set(&runtime_first_after_order,
				   atomic_inc(&runtime_hook_sequence) + 1);
		} else if (hook == runtime_second_after_hook) {
			atomic_set(&runtime_second_after_order,
				   atomic_inc(&runtime_hook_sequence) + 1);
		}
	}

	if ((hook == runtime_hook_to_block) && atomic_cas(&runtime_hook_enabled, 1, 0)) {
		k_sem_give(&runtime_hook_entered);
		(void)k_sem_take(&runtime_hook_continue, K_FOREVER);
	}
}

static int run_runtime_operation(const struct device *dev, enum test_runtime_operation operation)
{
	switch (operation) {
	case TEST_RUNTIME_GET:
		return pm_device_runtime_get(dev);
	case TEST_RUNTIME_PUT:
		return pm_device_runtime_put(dev);
	case TEST_RUNTIME_TURN_ON:
		return z_pm_device_runtime_power_domain_action_run(dev, PM_DEVICE_ACTION_TURN_ON);
	case TEST_RUNTIME_TURN_OFF:
		return z_pm_device_runtime_power_domain_action_run(dev, PM_DEVICE_ACTION_TURN_OFF);
	default:
		return -EINVAL;
	}
}

static enum z_pm_device_runtime_test_hook
operation_before_hook(enum test_runtime_operation operation)
{
	switch (operation) {
	case TEST_RUNTIME_GET:
		return Z_PM_DEVICE_RUNTIME_HOOK_BEFORE_GET;
	case TEST_RUNTIME_PUT:
		return Z_PM_DEVICE_RUNTIME_HOOK_BEFORE_PUT;
	case TEST_RUNTIME_TURN_ON:
	case TEST_RUNTIME_TURN_OFF:
		return Z_PM_DEVICE_RUNTIME_HOOK_BEFORE_PD_ACTION;
	default:
		CODE_UNREACHABLE;
	}
}

static enum z_pm_device_runtime_test_hook
operation_after_hook(enum test_runtime_operation operation)
{
	return operation_before_hook(operation) + 1;
}

static void runtime_operation_runner(void *context_ptr, void *arg2, void *arg3)
{
	struct test_runtime_operation_context *context = context_ptr;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	context->ret = run_runtime_operation(context->dev, context->operation);
	k_sem_give(&runtime_operation_done);
}

static void prepare_runtime_state(enum pm_device_state initial_state)
{
	zassert_ok(
		z_pm_device_runtime_power_domain_action_run(stress_dev, PM_DEVICE_ACTION_TURN_OFF));

	if (initial_state == PM_DEVICE_STATE_ACTIVE) {
		zassert_ok(z_pm_device_runtime_power_domain_action_run(stress_dev,
								       PM_DEVICE_ACTION_TURN_ON));
		zassert_ok(pm_device_runtime_get(stress_dev));
	}
}

static void run_runtime_ordering(enum pm_device_state initial_state,
				 enum test_runtime_operation first_operation, int first_expected,
				 enum test_runtime_operation second_operation, int second_expected,
				 enum pm_device_state final_state, int final_usage)
{
	struct test_runtime_operation_context context = {
		.dev = stress_dev,
		.operation = second_operation,
	};
	enum pm_device_state state;
	k_tid_t thread;
	int first_ret;

	atomic_clear(&runtime_hook_enabled);
	atomic_clear(&runtime_record_hooks);
	prepare_runtime_state(initial_state);
	emulated_pm_stress_callback_max_reset(stress_dev);
	k_sem_reset(&runtime_hook_entered);
	k_sem_reset(&runtime_hook_continue);
	k_sem_reset(&runtime_operation_done);
	atomic_clear(&runtime_hook_sequence);
	atomic_clear(&runtime_first_after_order);
	atomic_clear(&runtime_second_after_order);
	runtime_first_after_hook = operation_after_hook(first_operation);
	runtime_second_after_hook = operation_after_hook(second_operation);
	runtime_hook_to_block = operation_before_hook(second_operation);
	atomic_set(&runtime_record_hooks, 1);
	atomic_set(&runtime_hook_enabled, 1);

	thread = k_thread_create(&runtime_operation_thread, runtime_operation_stack,
				 K_THREAD_STACK_SIZEOF(runtime_operation_stack),
				 runtime_operation_runner, &context, NULL, NULL, K_PRIO_PREEMPT(2),
				 0, K_NO_WAIT);
	zassert_ok(k_sem_take(&runtime_hook_entered, K_MSEC(100)));
	zassert_equal(k_sem_take(&runtime_operation_done, K_NO_WAIT), -EBUSY);
	first_ret = run_runtime_operation(stress_dev, first_operation);
	zassert_equal(atomic_get(&runtime_first_after_order), 1);
	zassert_equal(atomic_get(&runtime_second_after_order), 0);
	if ((initial_state == PM_DEVICE_STATE_ACTIVE) && (first_operation == TEST_RUNTIME_GET) &&
	    (first_ret == 0)) {
		zassert_equal(pm_device_runtime_usage(stress_dev), 2);
	}
	k_sem_give(&runtime_hook_continue);
	zassert_ok(k_sem_take(&runtime_operation_done, K_MSEC(100)));
	zassert_ok(k_thread_join(thread, K_MSEC(100)));
	atomic_clear(&runtime_record_hooks);

	zassert_equal(first_ret, first_expected, "first operation %d", first_operation);
	zassert_equal(context.ret, second_expected, "second operation %d", second_operation);
	zassert_equal(atomic_get(&runtime_second_after_order), 2);
	zassert_ok(pm_device_state_get(stress_dev, &state));
	zassert_equal(state, final_state, "operations %d, %d", first_operation, second_operation);
	zassert_equal(pm_device_runtime_usage(stress_dev), final_usage, "operations %d, %d",
		      first_operation, second_operation);
	zassert_equal(emulated_pm_stress_callback_max_get(stress_dev), 1, "operations %d, %d",
		      first_operation, second_operation);
}

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

ZTEST(device_runtime_stress, test_power_domain_action_orderings)
{
	run_runtime_ordering(PM_DEVICE_STATE_ACTIVE, TEST_RUNTIME_TURN_OFF, 0, TEST_RUNTIME_GET,
			     -EAGAIN, PM_DEVICE_STATE_OFF, 0);
	run_runtime_ordering(PM_DEVICE_STATE_ACTIVE, TEST_RUNTIME_GET, 0, TEST_RUNTIME_TURN_OFF, 0,
			     PM_DEVICE_STATE_OFF, 0);
	run_runtime_ordering(PM_DEVICE_STATE_ACTIVE, TEST_RUNTIME_TURN_OFF, 0, TEST_RUNTIME_PUT,
			     -EALREADY, PM_DEVICE_STATE_OFF, 0);
	run_runtime_ordering(PM_DEVICE_STATE_ACTIVE, TEST_RUNTIME_PUT, 0, TEST_RUNTIME_TURN_OFF, 0,
			     PM_DEVICE_STATE_OFF, 0);
	run_runtime_ordering(PM_DEVICE_STATE_OFF, TEST_RUNTIME_TURN_ON, 0, TEST_RUNTIME_GET, 0,
			     PM_DEVICE_STATE_ACTIVE, 1);
	run_runtime_ordering(PM_DEVICE_STATE_OFF, TEST_RUNTIME_GET, -EAGAIN, TEST_RUNTIME_TURN_ON,
			     0, PM_DEVICE_STATE_SUSPENDED, 0);
	run_runtime_ordering(PM_DEVICE_STATE_OFF, TEST_RUNTIME_TURN_ON, 0, TEST_RUNTIME_PUT,
			     -EALREADY, PM_DEVICE_STATE_SUSPENDED, 0);
	run_runtime_ordering(PM_DEVICE_STATE_OFF, TEST_RUNTIME_PUT, -EALREADY, TEST_RUNTIME_TURN_ON,
			     0, PM_DEVICE_STATE_SUSPENDED, 0);
}

ZTEST_SUITE(device_runtime_stress, NULL, device_runtime_stress_setup, NULL, NULL,
	    device_runtime_stress_suite_teardown);
