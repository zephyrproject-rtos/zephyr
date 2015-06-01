/* timer.c - test microkernel timer APIs */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
This module tests the following microkernel timer routines:

  task_timer_alloc(), task_timer_free()
  task_timer_start(), task_timer_restart(), task_timer_stop()
  task_tick_delta(), task_tick_get_32()
*/

#include <tc_util.h>
#include <vxmicro.h>

extern struct nano_lifo _k_timer_free;    /* For white box testing only */

#define NTIMERS  CONFIG_NUM_TIMER_PACKETS

#define WITHIN_ERROR(var, target, epsilon)       \
		(((var) >= (target)) && ((var) <= (target) + (epsilon)))

static ktimer_t pTimer[NTIMERS + 1];

/*******************************************************************************
*
* testLowTimerStop - test that task_timer_stop() does stop a timer
*
* RETURNS: TC_PASS on success, TC_FAIL otherwise
*/

int testLowTimerStop(void)
{
	int  status;

	pTimer[0] = task_timer_alloc();

	task_timer_start(pTimer[0], 10, 5, TIMER_SEM);

	task_timer_stop(pTimer[0]);

	status = task_sem_take_wait_timeout(TIMER_SEM, 20);
	if (status != RC_TIME) {
		TC_ERROR("** task_sem_take_wait_timeout() returned %d, not %d\n", status, RC_TIME);
		return TC_FAIL;    /* Return failure, do not "clean up" */
	}

	task_timer_free(pTimer[0]);
	return TC_PASS;
}

/*******************************************************************************
*
* testLowTimerPeriodicity - test the periodic feature of a timer
*
* RETURNS: TC_PASS on success, TC_FAIL otherwise
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
	ticks_32 = task_tick_get_32();
	while (task_tick_get_32() == ticks_32) {
	}

	(void) task_tick_delta(&refTime);
	task_timer_start(pTimer[0], 100, 50, TIMER_SEM);

	for (i = 0; i < 5; i++) {
		status = task_sem_take_wait_timeout(TIMER_SEM, 200);
		ticks = task_tick_delta(&refTime);

		if (status != RC_OK) {
			TC_ERROR("** Timer appears to not have fired\n");
			return TC_FAIL;    /* Return failure, do not "clean up" */
		}

		if (((i == 0) && !WITHIN_ERROR(ticks, 100, 1)) ||
			((i != 0) && !WITHIN_ERROR(ticks, 50, 1))) {
			TC_ERROR("** Timer fired after %d ticks, not %d\n",
					 ticks, (i == 0) ? 100 : 50);
			return TC_FAIL;    /* Return failure, do not "clean up" */
		}
	}


	ticks_32 = task_tick_get_32();
	while (task_tick_get_32() == ticks_32) {     /* Align to a tick */
	}
	(void) task_tick_delta_32(&refTime);

	/* Use task_timer_restart() to change the periodicity */
	task_timer_restart(pTimer[0], 0, 60);
	for (i = 0; i < 6; i++) {
		status = task_sem_take_wait_timeout(TIMER_SEM, 100);
		ticks_32 = task_tick_delta_32(&refTime);

		if (status != RC_OK) {
			TC_ERROR("** Timer appears to not have fired\n");
			return TC_FAIL;    /* Return failure, do not "clean up" */
		}

		if (!WITHIN_ERROR(ticks_32, 60, 1)) {
			TC_ERROR("** Timer fired after %d ticks, not %d\n", ticks, 60);
			return TC_FAIL;    /* Return failure, do not "clean up" */
		}
	}

	/* task_timer_free() will both stop and free the timer */
	task_timer_free(pTimer[0]);
	return TC_PASS;
}

/*******************************************************************************
*
* testLowTimerDoesNotStart - test that the timer does not start
*
* This test checks that the timer does not start under a variety of
* circumstances.
*
* RETURNS: TC_PASS on success, TC_FAIL otherwise
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
		ticks = task_tick_get_32();
		while (task_tick_get_32() == ticks) {
		}

		task_timer_start(pTimer[0], Ti[i], Tr[i], TIMER_SEM);
		status = task_sem_take_wait_timeout(TIMER_SEM, 200);
		if (status != RC_TIME) {
			TC_ERROR("** Timer appears to have fired unexpectedly\n");
			return TC_FAIL;    /* Return failure, do not "clean up" */
		}

	}

	task_timer_free(pTimer[0]);
	return TC_PASS;
}

/*******************************************************************************
*
* testLowTimerOneShot - test the one shot feature of a timer
*
* RETURNS: TC_PASS on success, TC_FAIL otherwise
*/

int testLowTimerOneShot(void)
{
	int32_t    ticks;
	int64_t refTime;
	int        status;

	pTimer[0] = task_timer_alloc();

	/* Align to a tick */
	ticks = task_tick_get_32();
	while (task_tick_get_32() == ticks) {
	}

	/* Timer to fire once only in 100 ticks */
	(void) task_tick_delta(&refTime);
	task_timer_start(pTimer[0], 100, 0, TIMER_SEM);
	status = task_sem_take_wait(TIMER_SEM);
	ticks = task_tick_delta(&refTime);
	if (!WITHIN_ERROR(ticks, 100, 1)) {
		TC_ERROR("** Expected %d ticks to elapse, got %d\n", 100, ticks);
		return TC_FAIL;    /* Return failure, do not "clean up" */
	}
	if (status != RC_OK) {
		TC_ERROR("** task_sem_take_wait() unexpectedly failed\n");
		return TC_FAIL;    /* Return failure, do not "clean up" */
	}

	/*
	 * Wait up to 200 more ticks for another timer signalling
	 * that should not occur.
	 */
	status = task_sem_take_wait_timeout(TIMER_SEM, 200);
	if (status != RC_TIME) {
		TC_ERROR("** task_sem_take_wait_timeout() expected timeout,  got %d\n", status);
		return TC_FAIL;    /* Return failure, do not "clean up" */
	}

	task_timer_free(pTimer[0]);
	return TC_PASS;
}

/*******************************************************************************
*
* testLowTimerGet - test the task_timer_alloc() API
*
* This routine allocates all the timers in the system using task_timer_alloc().
* It verifies that all the allocated timers have unique IDs before freeing
* them using task_timer_free().
*
* This routine also does some partial testing of task_timer_free().  That is,
* it checks that timers that have been freed are available to be allocated
* again at a later time.
*
* RETURNS: TC_PASS on success, TC_FAIL otherwise
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

		if (_k_timer_free.list != NULL) {
			TC_ERROR("** Not all timers were allocated!\n");
		}

		for (i = 0; i < NTIMERS; i++) {
			task_timer_free(pTimer[i]);
		}
	}

	return TC_PASS;
}

/*******************************************************************************
*
* RegressionTaskEntry - regression test's entry point
*
* RETURNS: N/A
*/

void RegressionTaskEntry(void)
{
	int  tcRC;

	PRINT_DATA("Starting timer tests\n");
	PRINT_LINE;

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

exitRtn:
	TC_END_RESULT(tcRC);
	TC_END_REPORT(tcRC);
}
