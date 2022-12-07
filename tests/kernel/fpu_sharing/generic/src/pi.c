/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file
 * @brief pi computation portion of FPU sharing test
 *
 * @ingroup kernel_fpsharing_tests
 *
 * This module is used for the FPU sharing test, and supplements the basic
 * load/store test by incorporating two additional threads that utilize the
 * floating point unit.
 *
 * Testing utilizes a pair of tasks that independently compute pi. The lower
 * priority task is regularly preempted by the higher priority task, thereby
 * testing whether floating point context information is properly preserved.
 *
 * The following formula is used to compute pi:
 *
 *     pi = 4 * (1 - 1/3 + 1/5 - 1/7 + 1/9 - ... )
 *
 * This series converges to pi very slowly. For example, performing 50,000
 * iterations results in an accuracy of 3 decimal places.
 *
 * A reference value of pi is computed once at the start of the test. All
 * subsequent computations must produce the same value, otherwise an error
 * has occurred.
 */

#include <zephyr/ztest.h>

#include "float_context.h"
#include "test_common.h"

/*
 * PI_NUM_ITERATIONS: This macro is defined in the project's Makefile and
 * is configurable from the command line.
 */
static float reference_pi = 0.0f;

/*
 * Test counters are "volatile" because GCC wasn't properly updating
 * calc_pi_low_count properly when calculate_pi_low() contained a "return"
 * in its error handling logic -- the value was incremented in a register,
 * but never written back to memory. (Seems to be a compiler bug!)
 */
static volatile unsigned int calc_pi_low_count;
static volatile unsigned int calc_pi_high_count;

/* Indicates that the load/store test exited */
static volatile bool test_exited;

/* Semaphore for signaling end of test */
static K_SEM_DEFINE(test_exit_sem, 0, 1);

/**
 * @brief Entry point for the low priority pi compute task
 *
 * @ingroup kernel_fpsharing_tests
 */
static void calculate_pi_low(void)
{
	volatile float pi; /* volatile to avoid optimizing out of loop */
	float divisor = 3.0f;
	float sign = -1.0f;
	unsigned int ix;

	/* Loop until the test finishes, or an error is detected. */
	for (calc_pi_low_count = 0; !test_exited; calc_pi_low_count++) {

		sign = -1.0f;
		pi = 1.0f;
		divisor = 3.0f;

		for (ix = 0; ix < PI_NUM_ITERATIONS; ix++) {
			pi += sign / divisor;
			divisor += 2.0f;
			sign *= -1.0f;
		}

		pi *= 4.0f;

		if (reference_pi == 0.0f) {
			reference_pi = pi;
		} else if (reference_pi != pi) {
			printf("Computed pi %1.6f, reference pi %1.6f\n",
			       pi, reference_pi);
		}

		zassert_equal(reference_pi, pi,
			      "pi computation error");
	}
}

/**
 * @brief Entry point for the high priority pi compute task
 *
 * @ingroup kernel_fpsharing_tests
 */
static void calculate_pi_high(void)
{
	volatile float pi; /* volatile to avoid optimizing out of loop */
	float divisor = 3.0f;
	float sign = -1.0f;
	unsigned int ix;

	/* Run the test until the specified maximum test count is reached */
	for (calc_pi_high_count = 0;
	     calc_pi_high_count <= MAX_TESTS;
	     calc_pi_high_count++) {

		sign = -1.0f;
		pi = 1.0f;
		divisor = 3.0f;

		for (ix = 0; ix < PI_NUM_ITERATIONS; ix++) {
			pi += sign / divisor;
			divisor += 2.0f;
			sign *= -1.0f;
		}

		/*
		 * Relinquish the processor for the remainder of the current
		 * system clock tick, so that lower priority threads get a
		 * chance to run.
		 *
		 * This exercises the ability of the kernel to restore the
		 * FPU state of a low priority thread _and_ the ability of the
		 * kernel to provide a "clean" FPU state to this thread
		 * once the sleep ends.
		 */
		k_sleep(K_MSEC(10));

		pi *= 4.0f;

		if (reference_pi == 0.0f) {
			reference_pi = pi;
		} else if (reference_pi != pi) {
			printf("Computed pi %1.6f, reference pi %1.6f\n",
			       pi, reference_pi);
		}

		zassert_equal(reference_pi, pi,
			      "pi computation error");

		/* Periodically issue progress report */
		if ((calc_pi_high_count % 100) == 50) {
			printf("Pi calculation OK after %u (high) +"
			       " %u (low) tests (computed %1.6lf)\n",
			       calc_pi_high_count, calc_pi_low_count, pi);
		}
	}

	/* Signal end of test */
	test_exited = true;
	k_sem_give(&test_exit_sem);
}

K_THREAD_DEFINE(pi_low, THREAD_STACK_SIZE, calculate_pi_low, NULL, NULL, NULL,
		THREAD_LOW_PRIORITY, THREAD_FP_FLAGS, K_TICKS_FOREVER);

K_THREAD_DEFINE(pi_high, THREAD_STACK_SIZE, calculate_pi_high, NULL, NULL, NULL,
		THREAD_HIGH_PRIORITY, THREAD_FP_FLAGS, K_TICKS_FOREVER);

ZTEST(fpu_sharing_generic, test_pi)
{
	/* Initialise test states */
	test_exited = false;
	k_sem_reset(&test_exit_sem);

	/* Start test threads */
	k_thread_start(pi_low);
	k_thread_start(pi_high);

	/* Wait for test threads to exit */
	k_sem_take(&test_exit_sem, K_FOREVER);
}
