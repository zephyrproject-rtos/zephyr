/*
 * Copyright (c) 2022 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file
 * @brief complex number multiplication portion of DSP sharing test
 *
 * @ingroup kernel_dspsharing_tests
 *
 * This module is used for the DSP sharing test, and supplements the basic
 * load/store test by incorporating two additional threads that utilize the
 * DSP unit.
 *
 * Testing utilizes a pair of tasks that independently compute complex vector
 * dot product. The lower priority task is regularly preempted by the higher
 * priority task, thereby testing whether DSP context information is properly
 * preserved.
 *
 * A reference value of computed result is computed once at the start of the
 * test. All subsequent computations must produce the same value, otherwise
 * an error has occurred.
 */

#include <zephyr/ztest.h>
#include "fxarc.h"
#include "dsp_context.h"
#include "test_common.h"

/* stored in XY memory, need ARC_AGU_SHARING */
#define DATA_ATTR __xy __attribute__((section(".Xdata")))
static DATA_ATTR const cq15_t cq15_a[3] = {{0x20, 10}, {0x10, 20}, {4, 30}};
static DATA_ATTR const cq15_t cq15_b[3] = {{0x20, 11}, {0x10, 21}, {5, 31}};

static volatile short reference_result;

static volatile unsigned int calc_low_count;
static volatile unsigned int calc_high_count;

/* Indicates that the load/store test exited */
static volatile bool test_exited;

/* Semaphore for signaling end of test */
static K_SEM_DEFINE(test_exit_sem, 0, 1);

/**
 * @brief Entry point for the low priority compute task
 *
 * @ingroup kernel_dspsharing_tests
 */
static void calculate_low(void)
{
	volatile short res[2];
	/* Loop until the test finishes, or an error is detected. */
	for (calc_low_count = 0; !test_exited; calc_low_count++) {

		v2accum32_t acc = {0, 0};

		for (int i = 0; i < 3; i++) {
			acc = fx_v2a32_cmac_cq15(acc, cq15_a[i], cq15_b[i]);
		}
		/* cast reult from v2accum32_ to short type */
		res[0] = fx_q15_cast_asl_rnd_a32(fx_get_v2a32(acc, 0), 15);
		res[1] = fx_q15_cast_asl_rnd_a32(fx_get_v2a32(acc, 1), 15);

		if (reference_result == 0) {
			reference_result = res[0];
		} else if (reference_result != res[0]) {
			printf("Computed result %d, reference result %d\n",
			       res[0], reference_result);
		}

		zassert_equal(reference_result, res[0], "complex product computation error");
	}
}

/**
 * @brief Entry point for the high priority compute task
 *
 * @ingroup kernel_dspsharing_tests
 */
static void calculate_high(void)
{
	volatile short res[2];
	/* Run the test until the specified maximum test count is reached */
	for (calc_high_count = 0; calc_high_count <= MAX_TESTS; calc_high_count++) {

		v2accum32_t acc = {0, 0};

		for (int i = 0; i < 3; i++) {
			acc = fx_v2a32_cmac_cq15(acc, cq15_a[i], cq15_b[i]);
		}

		/*
		 * Relinquish the processor for the remainder of the current
		 * system clock tick, so that lower priority threads get a
		 * chance to run.
		 *
		 * This exercises the ability of the kernel to restore the
		 * DSP state of a low priority thread _and_ the ability of the
		 * kernel to provide a "clean" DSP state to this thread
		 * once the sleep ends.
		 */
		k_sleep(K_MSEC(10));

		res[0] = fx_q15_cast_asl_rnd_a32(fx_get_v2a32(acc, 0), 15);
		res[1] = fx_q15_cast_asl_rnd_a32(fx_get_v2a32(acc, 1), 15);

		if (reference_result == 0) {
			reference_result = res[0];
		} else if (reference_result != res[0]) {
			printf("Computed result %d, reference result %d\n",
			       res[0], reference_result);
		}

		zassert_equal(reference_result, res[0], "complex product computation error");

		/* Periodically issue progress report */
		if ((calc_high_count % 100) == 50) {
			printf("complex product calculation OK after %u (high) "
			       "+"
			       " %u (low) tests (computed %d)\n",
			       calc_high_count, calc_low_count, res[0]);
		}
	}

	/* Signal end of test */
	test_exited = true;
	k_sem_give(&test_exit_sem);
}

K_THREAD_DEFINE(cal_low, THREAD_STACK_SIZE, calculate_low, NULL, NULL, NULL,
		THREAD_LOW_PRIORITY, THREAD_DSP_FLAGS, K_TICKS_FOREVER);

K_THREAD_DEFINE(cal_high, THREAD_STACK_SIZE, calculate_high, NULL, NULL, NULL,
		THREAD_HIGH_PRIORITY, THREAD_DSP_FLAGS, K_TICKS_FOREVER);

ZTEST(dsp_sharing, test_calculation)
{
	/* Initialise test states */
	test_exited = false;
	k_sem_reset(&test_exit_sem);

	/* Start test threads */
	k_thread_start(cal_low);
	k_thread_start(cal_high);

	/* Wait for test threads to exit */
	k_sem_take(&test_exit_sem, K_FOREVER);
}
