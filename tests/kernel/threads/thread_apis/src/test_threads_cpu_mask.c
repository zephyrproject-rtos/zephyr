/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#include "tests_thread_apis.h"

/* Very simple (and limited) test of the SMP cpu mask API.  Runs on
 * just one CPU.  Creates a thread, sets the CPU mask, starts it,
 * checks if it ran (or didn't run) as expected.
 */

struct k_thread child_thread;

bool child_has_run;

void child_fn(void *a, void *b, void *c)
{
	child_has_run = true;
}


/**
 * @brief Verify that a thread's CPU mask decides whether it can be scheduled.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * The affinity APIs may only be applied to a thread that is prevented from
 * running, and the resulting mask decides whether the scheduler may dispatch
 * it at all. Both halves are checked: the calls are first refused on the
 * running caller, then a not-yet-started high priority thread is given each
 * shape of mask in turn and the test observes whether it runs when yielded to.
 * Skipped unless CONFIG_SCHED_CPU_MASK is enabled.
 *
 * Test steps:
 * - Call each mask API on the running thread itself and check the return.
 * - For each pass, create a higher priority thread in the K_FOREVER state and
 *   give it a mask: cleared, all enabled, CPU 0 disabled, or pinned to CPU 0.
 * - Start the thread, yield, and record whether it ran.
 * - Skip the pass that enables more than one CPU when PIN_ONLY is configured.
 *
 * Expected result:
 * - Every mask API returns -EINVAL for the running thread.
 * - The thread runs only for the masks that leave it eligible for this CPU.
 *
 * @see k_thread_cpu_mask_clear()
 * @see k_thread_cpu_mask_enable_all()
 * @see k_thread_cpu_mask_enable()
 * @see k_thread_cpu_mask_disable()
 * @see k_thread_cpu_pin()
 */
ZTEST(threads_lifecycle_1cpu, test_thread_cpu_mask)
{
#ifdef CONFIG_SCHED_CPU_MASK
	k_tid_t thread;
	int ret, pass, prio;

	/* Shouldn't be able to operate on a running thread */
	ret = k_thread_cpu_mask_clear(k_current_get());
	zassert_true(ret == -EINVAL, "");

	ret = k_thread_cpu_mask_enable_all(k_current_get());
	zassert_true(ret == -EINVAL, "");

	ret = k_thread_cpu_mask_enable(k_current_get(), 0);
	zassert_true(ret == -EINVAL, "");

	ret = k_thread_cpu_mask_disable(k_current_get(), 0);
	zassert_true(ret == -EINVAL, "");

	ret = k_thread_cpu_pin(k_current_get(), 0);
	zassert_true(ret == -EINVAL, "");

	for (pass = 0; pass < 4; pass++) {
		if (IS_ENABLED(CONFIG_SCHED_CPU_MASK_PIN_ONLY) && pass == 1) {
			/* Pass 1 enables more than one CPU in the
			 * mask, which is illegal when PIN_ONLY
			 */
			continue;
		}

		child_has_run = false;

		/* Create a thread at a higher priority, don't start
		 * it yet.
		 */
		prio = k_thread_priority_get(k_current_get());
		zassert_true(prio > K_HIGHEST_APPLICATION_THREAD_PRIO, "");
		thread = k_thread_create(&child_thread,
					 tstack, tstack_size,
					 child_fn, NULL, NULL, NULL,
					 K_HIGHEST_APPLICATION_THREAD_PRIO,
					 0, K_FOREVER);

		/* Set up the CPU mask */
		if (pass == 0) {
			ret = k_thread_cpu_mask_clear(thread);
			zassert_true(ret == 0, "");
		} else if (pass == 1) {
			ret = k_thread_cpu_mask_enable_all(thread);
			zassert_true(ret == 0, "");
		} else if (pass == 2) {
			ret = k_thread_cpu_mask_disable(thread, 0);
			zassert_true(ret == 0, "");
		} else {
			ret = k_thread_cpu_mask_enable(thread, 0);
			zassert_true(ret == 0, "");

			ret = k_thread_cpu_pin(thread, 0);
			zassert_true(ret == 0, "");
		}

		/* Start it.  If it is runnable, it will do so
		 * immediately when we yield.  Check to see if it ran.
		 */
		zassert_false(child_has_run, "");
		k_thread_start(thread);
		k_yield();

		if (pass == 1 || pass == 3) {
			zassert_true(child_has_run, "");
		} else {
			zassert_false(child_has_run, "");
		}

		k_thread_abort(thread);
	}
#else
	ztest_test_skip();
#endif
}
