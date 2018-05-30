/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <tc_util.h>
#include <ztest.h>
#include <kernel.h>
#include <kernel_structs.h>

#if CONFIG_MP_NUM_CPUS < 2
#error SMP test requires at least two CPUs!
#endif

#define T2_STACK_SIZE 2048

K_THREAD_STACK_DEFINE(t2_stack, T2_STACK_SIZE);

struct k_thread t2;

volatile int t2_count;

#define DELAY_US 50000

void t2_fn(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	t2_count = 0;

	/* This thread simply increments a counter while spinning on
	 * the CPU.  The idea is that it will always be iterating
	 * faster than the other thread so long as it is fairly
	 * scheduled (and it's designed to NOT be fairly schedulable
	 * without a separate CPU!), so the main thread can always
	 * check its progress.
	 */
	while (1) {
		k_busy_wait(DELAY_US);
		t2_count++;
	}
}

void test_main(void)
{
	int i, ok = 1;

	/* Sleep a bit to guarantee that both CPUs enter an idle
	 * thread from which they can exit correctly to run the main
	 * test.
	 */
	k_sleep(100);

	/* Create a thread at a fixed priority lower than the main
	 * thread.  In uniprocessor mode, this thread will never be
	 * scheduled and the test will fail.
	 */
	k_thread_create(&t2, t2_stack, T2_STACK_SIZE, t2_fn,
			NULL, NULL, NULL,
			CONFIG_MAIN_THREAD_PRIORITY + 1, 0, K_NO_WAIT);

	/* Wait for the other thread (on a separate CPU) to actually
	 * start running.  We want synchrony to be as perfect as
	 * possible.
	 */
	t2_count = -1;
	while (t2_count == -1) {
	}

	for (i = 0; i < 10; i++) {
		/* Wait slightly longer than the other thread so our
		 * count will always be lower
		 */
		k_busy_wait(DELAY_US + (DELAY_US / 8));

		if (t2_count <= i) {
			ok = 0;
			break;
		}
	}

	if (ok) {
		TC_END_REPORT(TC_PASS);
	} else {
		TC_END_REPORT(TC_FAIL);
	}
}
