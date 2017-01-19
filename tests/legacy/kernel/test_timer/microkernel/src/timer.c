/* timer.c - test microkernel timer APIs */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
DESCRIPTION
This module tests the following microkernel timer routines:

  task_timer_alloc(), task_timer_free()
  task_timer_start(), task_timer_restart(), task_timer_stop()
  sys_tick_delta(), sys_tick_get_32()
 */

#include <tc_util.h>
#include <util_test_common.h>
#include <zephyr.h>

#include "fifo_timeout.c"

extern bool _timer_pool_is_empty(void);    /* For white box testing only */
#define timer_pool_is_empty _timer_pool_is_empty

#define NTIMERS  CONFIG_NUM_TIMER_PACKETS

#define WITHIN_ERROR(var, target, epsilon)       \
		(((var) >= (target)) && ((var) <= (target) + (epsilon)))

static ktimer_t pTimer[NTIMERS + 1];

/**
 *
 * @brief Test that task_timer_stop() does stop a timer
 *
 * @return TC_PASS on success, TC_FAIL otherwise
 */

int testLowTimerStop(void)
{
	int  status;

	pTimer[0] = task_timer_alloc();

	task_timer_start(pTimer[0], 10, 5, TIMER_SEM);

	task_timer_stop(pTimer[0]);

	status = task_sem_take(TIMER_SEM, 20);
	if (status != RC_TIME) {
		TC_ERROR("** task_sem_take() returned %d, not %d\n", status, RC_TIME);
		return TC_FAIL;    /* Return failure, do not "clean up" */
	}

	task_timer_free(pTimer[0]);
	return TC_PASS;
}

/**
 *
 * @brief Test the periodic feature of a timer
 *
 * @return TC_PASS on success, TC_FAIL otherwise
 */

int testLowTimerPeriodicity(void)
{
	int64_t     ticks;
	int32_t     ticks_32;
	int64_t  refTime;
	int         i;
	int         status;

	pTimer[0] = task_timer_alloc();

	/* Align to a tick */
	ticks_32 = sys_tick_get_32();
	while (sys_tick_get_32() == ticks_32) {
	}

	(void) sys_tick_delta(&refTime);
	task_timer_start(pTimer[0], 100, 50, TIMER_SEM);

	for (i = 0; i < 5; i++) {
		status = task_sem_take(TIMER_SEM, 200);
		ticks = sys_tick_delta(&refTime);

		if (status != RC_OK) {
			TC_ERROR("** Timer appears to not have fired\n");
			return TC_FAIL;    /* Return failure, do not "clean up" */
		}

		if (((i == 0) && !WITHIN_ERROR(ticks, 100, 1)) ||
			((i != 0) && !WITHIN_ERROR(ticks, 50, 1))) {
			TC_ERROR("** Timer fired after %d ticks, not %d\n",
					 (int32_t)ticks, (i == 0) ? 100 : 50);
			return TC_FAIL;    /* Return failure, do not "clean up" */
		}
	}


	ticks_32 = sys_tick_get_32();
	while (sys_tick_get_32() == ticks_32) {     /* Align to a tick */
	}
	(void) sys_tick_delta_32(&refTime);

	/* Use task_timer_restart() to change the periodicity */
	task_timer_restart(pTimer[0], 60, 60);
	for (i = 0; i < 6; i++) {
		status = task_sem_take(TIMER_SEM, 100);
		ticks_32 = sys_tick_delta_32(&refTime);

		if (status != RC_OK) {
			TC_ERROR("** Timer appears to not have fired\n");
			return TC_FAIL;    /* Return failure, do not "clean up" */
		}

		if (!WITHIN_ERROR(ticks_32, 60, 1)) {
			TC_ERROR("** Timer fired after %d ticks, not %d\n",
				 (int32_t)ticks, 60);
			return TC_FAIL;    /* Return failure, do not "clean up" */
		}
	}

	/* task_timer_free() will both stop and free the timer */
	task_timer_free(pTimer[0]);
	return TC_PASS;
}

/**
 *
 * @brief Test that the timer does not start
 *
 * This test checks that the timer does not start under a variety of
 * circumstances.
 *
 * @return TC_PASS on success, TC_FAIL otherwise
 */

int testLowTimerDoesNotStart(void)
{
	int32_t    ticks;
	int        status;
	int        Ti[3] = {-1, 1, 0};
	int        Tr[3] = {1, -1, 0};
	int        i;

	pTimer[0] = task_timer_alloc();

	for (i = 0; i < 3; i++) {
		/* Align to a tick */
		ticks = sys_tick_get_32();
		while (sys_tick_get_32() == ticks) {
		}

		task_timer_start(pTimer[0], Ti[i], Tr[i], TIMER_SEM);
		status = task_sem_take(TIMER_SEM, 200);
		if (status != RC_TIME) {
			TC_ERROR("** Timer appears to have fired unexpectedly\n");
			return TC_FAIL;    /* Return failure, do not "clean up" */
		}

	}

	task_timer_free(pTimer[0]);
	return TC_PASS;
}

/**
 *
 * @brief Test the one shot feature of a timer
 *
 * @return TC_PASS on success, TC_FAIL otherwise
 */

int testLowTimerOneShot(void)
{
	int32_t    ticks;
	int64_t refTime;
	int        status;

	pTimer[0] = task_timer_alloc();

	/* Align to a tick */
	ticks = sys_tick_get_32();
	while (sys_tick_get_32() == ticks) {
	}

	/* Timer to fire once only in 100 ticks */
	(void) sys_tick_delta(&refTime);
	task_timer_start(pTimer[0], 100, 0, TIMER_SEM);
	status = task_sem_take(TIMER_SEM, TICKS_UNLIMITED);
	ticks = sys_tick_delta(&refTime);
	if (!WITHIN_ERROR(ticks, 100, 1)) {
		TC_ERROR("** Expected %d ticks to elapse, got %d\n", 100, ticks);
		return TC_FAIL;    /* Return failure, do not "clean up" */
	}
	if (status != RC_OK) {
		TC_ERROR("** task_sem_take() unexpectedly failed\n");
		return TC_FAIL;    /* Return failure, do not "clean up" */
	}

	/*
	 * Wait up to 200 more ticks for another timer signalling
	 * that should not occur.
	 */
	status = task_sem_take(TIMER_SEM, 200);
	if (status != RC_TIME) {
		TC_ERROR("** task_sem_take() expected timeout,  got %d\n", status);
		return TC_FAIL;    /* Return failure, do not "clean up" */
	}

	task_timer_free(pTimer[0]);
	return TC_PASS;
}

/**
 *
 * @brief Test the task_timer_alloc() API
 *
 * This routine allocates all the timers in the system using task_timer_alloc().
 * It verifies that all the allocated timers have unique IDs before freeing
 * them using task_timer_free().
 *
 * This routine also does some partial testing of task_timer_free().  That is,
 * it checks that timers that have been freed are available to be allocated
 * again at a later time.
 *
 * @return TC_PASS on success, TC_FAIL otherwise
 */

int testLowTimerGet(void)
{
	int  i;
	int  j;
	int  k;

	for (j = 0; j < 2; j++) {
		for (i = 0; i < NTIMERS; i++) {
			pTimer[i] = task_timer_alloc();

			for (k = 0; k < i; k++) {
				if (pTimer[i] == pTimer[k]) {
					TC_ERROR("** task_timer_alloc() did not return a unique "
							"timer ID.\n");
					return TC_FAIL;
				}
			}
		}

		/* Whitebox test to ensure that all timers were allocated. */

		if (!timer_pool_is_empty()) {
			TC_ERROR("** Not all timers were allocated!\n");
		}

		for (i = 0; i < NTIMERS; i++) {
			task_timer_free(pTimer[i]);
		}
	}

	return TC_PASS;
}

extern int test_fifo_timeout(void);
void test_nano_timeouts(void)
{
	if (test_fifo_timeout() == TC_PASS) {
		task_sem_give(test_nano_timeouts_sem);
	}

	/* on failure, don't give semaphore, main test will time out */
}

#define TEST_NANO_TIMERS_DELAY 4
static struct nano_sem test_nano_timers_sem;
static char __stack test_nano_timers_stack[512];
static void test_nano_timers(int unused1, int unused2)
{
	struct nano_timer timer;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	nano_timer_init(&timer, (void *)0xdeadbeef);
	TC_PRINT("starting nano timer to expire in %d seconds\n",
				TEST_NANO_TIMERS_DELAY);
	nano_fiber_timer_start(&timer, SECONDS(TEST_NANO_TIMERS_DELAY));
	TC_PRINT("fiber pending on timer\n");
	nano_fiber_timer_test(&timer, TICKS_UNLIMITED);
	TC_PRINT("fiber back from waiting on timer: giving semaphore.\n");
	nano_task_sem_give(&test_nano_timers_sem);
	TC_PRINT("fiber semaphore given.\n");

	/* on failure, don't give semaphore, main test will not obtain it */
}

/**
 *
 * @brief Regression test's entry point
 *
 * @return N/A
 */

void RegressionTaskEntry(void)
{
	int  tcRC;

	nano_sem_init(&test_nano_timers_sem);

	PRINT_DATA("Starting timer tests\n");
	PRINT_LINE;

	task_fiber_start(test_nano_timers_stack, 512, test_nano_timers, 0, 0, 5, 0);

	/* Test the task_timer_alloc() API */

	TC_PRINT("Test the allocation of timers\n");
	tcRC = testLowTimerGet();
	if (tcRC != TC_PASS) {
		goto exitRtn;
	}

	TC_PRINT("Test the one shot feature of a timer\n");
	tcRC = testLowTimerOneShot();
	if (tcRC != TC_PASS) {
		goto exitRtn;
	}

	TC_PRINT("Test that a timer does not start\n");
	tcRC = testLowTimerDoesNotStart();
	if (tcRC != TC_PASS) {
		goto exitRtn;
	}

	TC_PRINT("Test the periodic feature of a timer\n");
	tcRC = testLowTimerPeriodicity();
	if (tcRC != TC_PASS) {
		goto exitRtn;
	}

	TC_PRINT("Test the stopping of a timer\n");
	tcRC = testLowTimerStop();
	if (tcRC != TC_PASS) {
		goto exitRtn;
	}

	TC_PRINT("Verifying the nanokernel timer fired\n");
	if (!nano_task_sem_take(&test_nano_timers_sem, TICKS_NONE)) {
		tcRC = TC_FAIL;
		goto exitRtn;
	}

	TC_PRINT("Verifying the nanokernel timeouts worked\n");
	tcRC = task_sem_take(test_nano_timeouts_sem, SECONDS(5));
	tcRC = tcRC == RC_OK ? TC_PASS : TC_FAIL;

exitRtn:
	TC_END_RESULT(tcRC);
	TC_END_REPORT(tcRC);
}
