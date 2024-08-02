/*
 * Copyright (c) 2022, Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file
 * @brief load/store portion of DSP sharing test
 *
 * @ingroup all_tests
 *
 * This module implements the load/store portion of the  DSP sharing test. This
 * version of this test utilizes a pair of tasks.
 *
 * The load/store test validates the dsp unit context
 * save/restore mechanism. This test utilizes a pair of threads of different
 * priorities that each use the dsp registers. The context
 * switching that occurs exercises the kernel's ability to properly preserve
 * the dsp registers. The test also exercises the kernel's ability
 * to automatically enable dsp support for a task, if supported.
 *
 */

#include <zephyr/ztest.h>
#include <zephyr/debug/gcov.h>
#include "dsp_regs_arc.h"
#include "dsp_context.h"
#include "test_common.h"

/* space for dsp register load/store area used by low priority task */
static struct dsp_register_set dsp_reg_set_load;
static struct dsp_register_set dsp_reg_set_store;

/* space for dsp register load/store area used by high priority thread */
static struct dsp_register_set dsp_reg_set;

static volatile unsigned int load_store_low_count;
static volatile unsigned int load_store_high_count;

/* Indicates that the load/store test exited */
static volatile bool test_exited;

/* Semaphore for signaling end of test */
static K_SEM_DEFINE(test_exit_sem, 0, 1);

/**
 * @brief Low priority DSP load/store thread
 *
 * @ingroup kernel_dspsharing_tests
 *
 * @see k_sched_time_slice_set(), memset(),
 * _load_all_dsp_registers(), _store_all_dsp_registers()
 */
static void load_store_low(void)
{
	volatile unsigned int i;
	bool error = false;
	unsigned char init_byte;
	unsigned char *store_ptr = (unsigned char *)&dsp_reg_set_store;
	unsigned char *load_ptr = (unsigned char *)&dsp_reg_set_load;
	/*
	 * Initialize dsp load buffer to known values;
	 * these values must be different than the value used in other threads.
	 */
	init_byte = MAIN_DSP_REG_CHECK_BYTE;
	for (i = 0; i < SIZEOF_DSP_REGISTER_SET; i++) {
		load_ptr[i] = init_byte++;
	}

	/* Loop until the test finishes, or an error is detected. */
	for (load_store_low_count = 0; !test_exited; load_store_low_count++) {

		/*
		 * Clear store buffer to erase all traces of any previous
		 * dsp values that have been saved.
		 */
		(void)memset(&dsp_reg_set_store, 0, SIZEOF_DSP_REGISTER_SET);

		/*
		 * Utilize an architecture specific function to load all the
		 * dsp registers with known values.
		 */
		_load_all_dsp_registers(&dsp_reg_set_load);

		/*
		 * Waste some cycles to give the high priority load/store
		 * thread an opportunity to run when the low priority thread is
		 * using the dsp registers.
		 *
		 * IMPORTANT: This logic requires that sys_clock_tick_get_32() not
		 * perform any dsp operations!
		 */
		k_busy_wait(100);

		/*
		 * Utilize an architecture specific function to dump the
		 * contents of all dsp registers to memory.
		 */
		_store_all_dsp_registers(&dsp_reg_set_store);
		/*
		 * Compare each byte of buffer to ensure the expected value is
		 * present, indicating that the dsp registers weren't
		 * impacted by the operation of the high priority thread(s).
		 *
		 * Display error message and terminate if discrepancies are
		 * detected.
		 */
		init_byte = MAIN_DSP_REG_CHECK_BYTE;

		for (i = 0; i < SIZEOF_DSP_REGISTER_SET; i++) {
			if (store_ptr[i] != init_byte) {
				TC_ERROR("Found 0x%x instead of 0x%x @offset 0x%x\n",
					 store_ptr[i], init_byte, i);
				TC_ERROR("Discrepancy found during iteration %d\n",
					 load_store_low_count);
				error = true;
			}
			init_byte++;
		}

		/* Terminate if a test error has been reported */
		zassert_false(error, NULL);
	}
}

/**
 * @brief High priority DSP load/store thread
 *
 * @ingroup kernel_dspsharing_tests
 *
 * @see _load_then_store_all_dsp_registers()
 */
static void load_store_high(void)
{
	volatile unsigned int i;
	unsigned char init_byte;
	unsigned char *reg_set_ptr = (unsigned char *)&dsp_reg_set;

	/* Run the test until the specified maximum test count is reached */
	for (load_store_high_count = 0;
	     load_store_high_count <= MAX_TESTS;
	     load_store_high_count++) {

		/*
		 * Initialize the dsp_reg_set structure by treating it as
		 * a simple array of bytes (the arrangement and actual number
		 * of registers is not important for this generic C code).  The
		 * structure is initialized by using the byte value specified
		 * by the constant FIBER_DSP_REG_CHECK_BYTE, and then
		 * incrementing the value for each successive location in the
		 * dsp_reg_set structure.
		 *
		 * The initial byte value, and thus the contents of the entire
		 * dsp_reg_set structure, must be different for each
		 * thread to effectively test the kernel's ability to
		 * properly save/restore the dsp-processed values during a
		 * context switch.
		 */
		init_byte = FIBER_DSP_REG_CHECK_BYTE;

		for (i = 0; i < SIZEOF_DSP_REGISTER_SET; i++) {
			reg_set_ptr[i] = init_byte++;
		}

		/*
		 * Utilize an architecture specific function to load all the
		 * dsp registers with the contents of the
		 * dsp_reg_set structure.
		 *
		 * The goal of the loading all dsp registers with
		 * values that differ from the values used in other threads is
		 * to help determine whether the dsp register
		 * save/restore mechanism in the kernel's context switcher
		 * is operating correctly.
		 *
		 * When a subsequent k_timer_test() invocation is
		 * performed, a (cooperative) context switch back to the
		 * preempted task will occur. This context switch should result
		 * in restoring the state of the task's dsp
		 * registers when the task was swapped out due to the
		 * occurrence of the timer tick.
		 */
		_load_then_store_all_dsp_registers(&dsp_reg_set);

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
		k_sleep(K_MSEC(1));

		/* Periodically issue progress report */
		if ((load_store_high_count % 100) == 0) {
			PRINT_DATA("Load and store OK after %u (high) "
				   "+ %u (low) tests\n",
				   load_store_high_count,
				   load_store_low_count);
		}
	}

	/* Signal end of test */
	test_exited = true;
	k_sem_give(&test_exit_sem);
}

K_THREAD_DEFINE(load_low, THREAD_STACK_SIZE, load_store_low, NULL, NULL, NULL,
		THREAD_LOW_PRIORITY, THREAD_DSP_FLAGS, K_TICKS_FOREVER);

K_THREAD_DEFINE(load_high, THREAD_STACK_SIZE, load_store_high, NULL, NULL, NULL,
		THREAD_HIGH_PRIORITY, THREAD_DSP_FLAGS, K_TICKS_FOREVER);

ZTEST(dsp_sharing, test_load_store)
{
	/* Initialise test states */
	test_exited = false;
	k_sem_reset(&test_exit_sem);

	/* Start test threads */
	k_thread_start(load_low);
	k_thread_start(load_high);

	/* Wait for test threads to exit */
	k_sem_take(&test_exit_sem, K_FOREVER);
}
