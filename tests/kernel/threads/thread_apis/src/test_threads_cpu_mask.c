/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>
#include <kernel.h>

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

void test_threads_cpu_mask(void)
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

	for (pass = 0; pass < 4; pass++) {
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
#endif
}
