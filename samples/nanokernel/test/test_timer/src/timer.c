/* timer.c - test timer APIs under VxMicro */

/*
 * Copyright (c) 2012-2015 Wind River Systems, Inc.
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
This module tests the following timer related routines:
  nano_timer_init(), nano_fiber_timer_start(), nano_fiber_timer_stop(),
  nano_fiber_timer_test(), nano_fiber_timer_wait(), nano_task_timer_start(),
  nano_task_timer_stop(), nano_task_timer_test(), nano_task_timer_wait(),
  nano_time_init(), nano_tick_get_32(), nano_cycle_get_32(), nano_tick_delta()
*/

/* includes */

#include <tc_util.h>
#include <nanokernel/cpu.h>

/* defines */

#define TWO_SECONDS     (2 * sys_clock_ticks_per_sec)
#define SIX_SECONDS     (6 * sys_clock_ticks_per_sec)

#define SHORT_TIMEOUT   (1 * sys_clock_ticks_per_sec)
#define LONG_TIMEOUT    (5 * sys_clock_ticks_per_sec)
#define MID_TIMEOUT     (3 * sys_clock_ticks_per_sec)

#define FIBER_STACKSIZE    2000
#define FIBER_PRIORITY     4

#define FIBER2_STACKSIZE   2000
#define FIBER2_PRIORITY    10

/* typedefs */

typedef void  (* timer_start_func)(struct nano_timer *, int);
typedef void  (* timer_stop_func)(struct nano_timer *);
typedef void* (* timer_getw_func)(struct nano_timer *);
typedef void* (* timer_get_func)(struct nano_timer *);

/* locals */

static struct nano_timer  timer;
static struct nano_timer  shortTimer;
static struct nano_timer  longTimer;
static struct nano_timer  midTimer;

static struct nano_sem    wakeTask;
static struct nano_sem    wakeFiber;

static void *timerData[1];
static void *shortTimerData[1];
static void *longTimerData[1];
static void *midTimerData[1];

static int  fiberDetectedError = 0;
static char fiberStack[FIBER_STACKSIZE];
static char fiber2Stack[FIBER2_STACKSIZE];

/*******************************************************************************
*
* initNanoObjects - initialize nanokernel objects
*
* This routine initializes the nanokernel objects used in the LIFO tests.
*
* RETURNS: N/A
*/

void initNanoObjects(void)
{
	nano_timer_init(&timer, timerData);
	nano_timer_init(&shortTimer, shortTimerData);
	nano_timer_init(&longTimer, longTimerData);
	nano_timer_init(&midTimer, midTimerData);
	nano_sem_init(&wakeTask);
	nano_sem_init(&wakeFiber);
}

/*******************************************************************************
*
* basicTimerWait - basic checking of time spent waiting upon a timer
*
* This routine can be called from a task or a fiber to wait upon a timer.
* It will busy wait until the current tick ends, at which point it will
* start and then wait upon a timer.  The length of time it spent waiting
* gets cross-checked with the nano_tick_get_32() and nanoTimeElapsed() APIs.
* All three are expected to match up, but a tolerance of one (1) tick is
* considered acceptable.
*
* This routine can be considered as testing nano_tick_get_32(),
* nanoTimeElapsed() and nanoXXXTimerGetW() successful expiration cases.
*
* \param startRtn      routine to start the timer
* \param waitRtn       routine to get and wait for the timer
* \param getRtn        routine to get the timer (no waiting)
* \param pTimer        pointer to the timer
* \param pTimerData    pointer to the expected timer data
* \param ticks         number of ticks to wait
*
* RETURNS: TC_PASS on success, TC_FAIL on failure
*/

int basicTimerWait(timer_start_func startRtn, timer_getw_func waitRtn,
				   timer_get_func getRtn, struct nano_timer *pTimer,
				   void *pTimerData, int ticks)
{
	int64_t  reftime;          /* reference time for tick delta */
	uint32_t  tick;            /* current tick */
	uint32_t  elapsed_32;      /* # of elapsed ticks for 32-bit functions*/
	int64_t  elapsed;          /* # of elapsed ticks */
	uint32_t  duration;        /* duration of the test in ticks */
	void     *result;          /* value returned from timer get routine */
	int       busywaited = 0;  /* non-zero if <getRtn> returns NULL */

	TC_PRINT("  - test expected to take four seconds\n");

	tick = nano_tick_get_32();
	while (nano_tick_get_32() == tick) {
        /* Align to a tick boundary */
		}

	tick++;
	(void) nano_tick_delta (&reftime);
	startRtn(pTimer, ticks);       /* Start the timer */
	result = waitRtn(pTimer);      /* Wait for the timer to expire */

	elapsed_32 = nano_tick_delta_32(&reftime);
	duration = nano_tick_get_32() - tick;

    /*
     * The difference between <duration> and <elapsed> is expected to be zero
     * however, the test is allowing for tolerance of an extra tick in case of
     * timing variations.
     */

	if ((result != pTimerData) ||
		(duration - elapsed_32 > 1) || ((duration - ticks) > 1)) {
		return TC_FAIL;
		}

    /* Check that the non-wait-timer-get routine works properly. */
	tick = nano_tick_get_32();
	while (nano_tick_get_32() == tick) {
        /* Align to a tick boundary */
		}

	tick++;
	(void) nano_tick_delta (&reftime);
	startRtn(pTimer, ticks);       /* Start the timer */
	while ((result = getRtn (pTimer)) == NULL) {
		busywaited = 1;
		}
	elapsed = nano_tick_delta(&reftime);
	duration = nano_tick_get_32() - tick;

	if ((busywaited != 1) || (result != pTimerData) ||
		(duration - elapsed > 1) || ((duration - ticks) > 1)) {
		return TC_FAIL;
		}

	return TC_PASS;
}

/*******************************************************************************
*
* startTimers - start four timers
*
* This routine starts four timers.
* The first (<timer>) is added to an empty list of timers.
* The second (<longTimer>) is added to the end of the list of timers.
* The third (<shortTimer>) is added to the head of the list of timers.
* The fourth (<midTimer>) is added to the middle of the list of timers.
*
* Four timers are used so that the various paths can be tested.
*
* \param startRtn    routine to start the timers
*
* RETURNS: N/A
*/

void startTimers(timer_start_func startRtn)
{
	int  tick;                    /* current tick */

	tick = nano_tick_get_32();
	while (nano_tick_get_32 () == tick) {
        /* Wait for the end of the tick */
		}

	startRtn(&timer, TWO_SECONDS);
	startRtn(&longTimer, LONG_TIMEOUT);
	startRtn(&shortTimer, SHORT_TIMEOUT);
	startRtn(&midTimer, MID_TIMEOUT);
}

/*******************************************************************************
*
* busyWaitTimers - busy wait while checking timers expire in the correct order
*
* This routine checks that the four timers created using startTimers() finish
* in the correct order.  It busy waits on all four timers waiting until they
* expire.  The timers are expected to expire in the following order:
*     <shortTimer>, <timer>, <midTimer>, <longTimer>
*
* \param getRtn    timer get routine (fiber or task)
*
* RETURNS: TC_PASS on success, TC_FAIL on failure
*/

int busyWaitTimers(timer_get_func getRtn)
{
	int      numExpired = 0; /* # of expired timers */
	void    *result;         /* value returned from <getRtn> */
	uint32_t ticks;          /* tick by which time test should be complete */

	TC_PRINT("  - test expected to take five or six seconds\n");

	ticks = nano_tick_get_32() + SIX_SECONDS;
	while ((numExpired != 4) && (nano_tick_get_32 () < ticks)) {
		result = getRtn(&timer);
		if (result != NULL) {
		    numExpired++;
		    if ((result != timerData) || (numExpired != 2)) {
		        TC_ERROR("Expected <timer> to expire 2nd, not 0x%x\n",
		                  result);
		        return TC_FAIL;
		        }
		    }

		result = getRtn(&shortTimer);
		if (result != NULL) {
		    numExpired++;
		    if ((result != shortTimerData) || (numExpired != 1)) {
		        TC_ERROR("Expected <shortTimer> to expire 1st, not 0x%x\n",
		                  result);
		        return TC_FAIL;
		        }
		    }

		result = getRtn(&midTimer);
		if (result != NULL) {
		    numExpired++;
		    if ((result != midTimerData) || (numExpired != 3)) {
		        TC_ERROR("Expected <midTimer> to expire 3rd, not 0x%x\n",
		                  result);
		        return TC_FAIL;
		        }
		    }

		result = getRtn(&longTimer);
		if (result != NULL) {
		    numExpired++;
		    if ((result != longTimerData) || (numExpired != 4)) {
		        TC_ERROR("Expected <longTimer> to expire 4th, not 0x%x\n",
		                  result);
		        return TC_FAIL;
		        }
		    }
		}

	return (nano_tick_get_32 () < ticks) ? TC_PASS : TC_FAIL;
}

/*******************************************************************************
*
* stopTimers - stop the four timers and make sure they did not expire
*
* This routine stops the four started timers and then checks the timers for
* six seconds to make sure that they did not fire.  The four timers will be
* stopped in the reverse order in which they were started.  Doing so will
* exercise the code that removes timers from important locations in the list;
* these include the middle, the head, the tail, and the last item.
*
* \param stopRtn    routine to stop timer (fiber or task)
* \param getRtn     timer get routine (fiber or task)
*
* RETURNS: TC_PASS on success, TC_FAIL on failure
*/

int stopTimers(timer_stop_func stopRtn, timer_get_func getRtn)
{
	int  startTick;      /* tick at which test starts */
	int  endTick;        /* tick by which test should be completed */

	stopRtn(&midTimer);
	stopRtn(&shortTimer);
	stopRtn(&longTimer);
	stopRtn(&timer);

	TC_PRINT("  - test expected to take six seconds\n");

	startTick = nano_tick_get_32();
	while (nano_tick_get_32 () == startTick) {
		}
	startTick++;
	endTick = startTick + SIX_SECONDS;

	while (nano_tick_get_32 () < endTick) {
		if ((getRtn (&timer) != NULL) || (getRtn (&shortTimer) != NULL) ||
		    (getRtn (&midTimer) != NULL) || (getRtn (&longTimer) != NULL)) {
		    return TC_FAIL;
		    }
		}

	return TC_PASS;
}

/*******************************************************************************
*
* fiber2Entry - entry point for the second fiber
*
* The second fiber has a lower priority than the first, but is still given
* precedence over the task.
*
* \param arg1    unused
* \param arg2    unused
*
* RETURNS: N/A
*/

static void fiber2Entry(int arg1, int arg2)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	nano_fiber_timer_stop(&timer);
}

/*******************************************************************************
*
* fiberEntry - entry point for the fiber portion of the timer tests
*
* NOTE: The fiber portion of the tests have higher priority than the task
* portion of the tests.
*
* \param arg1    unused
* \param arg2    unused
*
* RETURNS: N/A
*/

static void fiberEntry(int arg1, int arg2)
{
	int  rv;      /* return value from a test */
	void *result; /* return value from timer wait routine */

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	TC_PRINT("Fiber testing basic timer functionality\n");

	rv = basicTimerWait(nano_fiber_timer_start, nano_fiber_timer_wait,
		                 nano_fiber_timer_test, &timer, timerData, TWO_SECONDS);

	nano_fiber_sem_give(&wakeTask);
	if (rv != TC_PASS) {
		fiberDetectedError = 1;
		return;
		}
	nano_fiber_sem_take_wait(&wakeFiber);    /* Wait forever - let task run */

    /* Check that timers expire in the correct order */
	TC_PRINT("Fiber testing timers expire in the correct order\n");
	startTimers(nano_fiber_timer_start);
	rv = busyWaitTimers(nano_fiber_timer_test);
	nano_fiber_sem_give(&wakeTask);
	if (rv != TC_PASS) {
		fiberDetectedError = 2;
		return;
		}
	nano_fiber_sem_take_wait(&wakeFiber);    /* Wait forever - let task run */

    /* Check that timers can be stopped */
	TC_PRINT("Task testing the stopping of timers\n");
	startTimers(nano_fiber_timer_start);
	rv = stopTimers(nano_fiber_timer_stop, nano_fiber_timer_test);
	nano_fiber_sem_give(&wakeTask);
	if (rv != TC_PASS) {
		fiberDetectedError = 3;
		return;
		}
	nano_fiber_sem_take_wait(&wakeFiber);    /* Wait forever - let task run */

    /* Fiber to wait on a timer that will be stopped by another fiber */
	TC_PRINT("Fiber to stop a timer that has a waiting fiber\n");
	fiber_fiber_start(fiber2Stack, FIBER2_STACKSIZE, fiber2Entry,
		                 0, 0, FIBER2_PRIORITY, 0);
	nano_fiber_timer_start(&timer, TWO_SECONDS);   /* Start timer */
	result = nano_fiber_timer_wait(&timer);        /* Wait on timer */
    /* Control switches to newly created fiber #2 before coming back. */
	if (result != NULL) {
		fiberDetectedError = 4;
		nano_fiber_sem_give(&wakeTask);
		return;
		}

    /* Fiber to wait on timer that will be stopped by the task */
	TC_PRINT("Task to stop a timer that has a waiting fiber\n");
	nano_fiber_sem_give(&wakeTask);
	nano_fiber_timer_start(&timer, TWO_SECONDS);
	result = nano_fiber_timer_wait(&timer);
	if (result != NULL) {
		fiberDetectedError = 5;
		return;
		}

	nano_fiber_sem_give(&wakeTask);
}

/*******************************************************************************
*
* nano_cycle_get_32Test - test the nano_cycle_get_32() API
*
* RETURNS: TC_PASS on success, TC_FAIL on failure
*/

int nano_cycle_get_32Test(void)
{
	uint32_t  timeStamp1;
	uint32_t  timeStamp2;
	int       i;

	timeStamp2 = nano_cycle_get_32();
	for (i = 0; i < 1000000; i++) {
		timeStamp1 = timeStamp2;
		timeStamp2 = nano_cycle_get_32();

		if (timeStamp2 < timeStamp1) {
		    TC_ERROR("Timestamp value not increasing with successive calls\n");
		    return TC_FAIL;
		    }
		}

	return TC_PASS;
}

/*******************************************************************************
*
* main - entry point to timer tests
*
* This is the entry point to the timer tests.
*
* RETURNS: N/A
*/

void main(void)
{
	int     rv;       /* return value from tests */

	TC_START("Test Nanokernel Timer");

	initNanoObjects();

	TC_PRINT("Task testing basic timer functionality\n");
	rv = basicTimerWait(nano_task_timer_start, nano_task_timer_wait,
		                 nano_task_timer_test, &timer, timerData, TWO_SECONDS);
	if (rv != TC_PASS) {
		TC_ERROR("Task-level of waiting for timers failed\n");
		goto doneTests;
		}

    /* Check that timers expire in the correct order */
	TC_PRINT("Task testing timers expire in the correct order\n");
	startTimers(nano_task_timer_start);
	rv = busyWaitTimers(nano_task_timer_test);
	if (rv != TC_PASS) {
		TC_ERROR("Task-level timer expiration order failed\n");
		goto doneTests;
		}

    /* Check that timers can be stopped */
	TC_PRINT("Task testing the stopping of timers\n");
	startTimers(nano_task_timer_start);
	rv = stopTimers(nano_task_timer_stop, nano_task_timer_test);
	if (rv != TC_PASS) {
		TC_ERROR("Task-level stopping of timers test failed\n");
		goto doneTests;
		}

    /*
     * Start the fiber.  The fiber will be given a higher priority than the
     * main task.
     */

	task_fiber_start(fiberStack, FIBER_STACKSIZE, fiberEntry,
		                0, 0, FIBER_PRIORITY, 0);

	nano_task_sem_take_wait(&wakeTask);

	if (fiberDetectedError == 1) {
		TC_ERROR("Fiber-level of waiting for timers failed\n");
		rv = TC_FAIL;
		goto doneTests;
		}

	nano_task_sem_give(&wakeFiber);
	nano_task_sem_take_wait(&wakeTask);

	if (fiberDetectedError == 2) {
		TC_ERROR("Fiber-level timer expiration order failed\n");
		rv = TC_FAIL;
		goto doneTests;
		}

	nano_task_sem_give(&wakeFiber);
	nano_task_sem_take_wait(&wakeTask);

	if (fiberDetectedError == 3) {
		TC_ERROR("Fiber-level stopping of timers test failed\n");
		rv = TC_FAIL;
		goto doneTests;
		}

	nano_task_sem_give(&wakeFiber);
	nano_task_sem_take_wait(&wakeTask);
	if (fiberDetectedError == 4) {
		TC_ERROR("Fiber stopping a timer waited upon by a fiber failed\n");
		rv = TC_FAIL;
		goto doneTests;
		}
	nano_task_timer_stop(&timer);

	if (fiberDetectedError == 5) {
		TC_ERROR("Task stopping a timer waited upon by a fiber failed\n");
		rv = TC_FAIL;
		goto doneTests;
		}

	nano_task_sem_take_wait(&wakeTask);

#if 0
    /*
     * Due to recent changes in the i8253 file that correct an issue on real
     * hardware, this test will fail when run under QEMU.  On QEMU, the i8253
     * timer can at appear to run backwards.  This can generate a false
     * failure detection when this test is run under QEMU as part of the
     * standard sanity/regression checks.  This suggests that the test is not
     * of high enough quality to be included during the standard sanity/
     * regression checks.
     */

	TC_PRINT("Task testing of nano_cycle_get_32()\n");
	rv = nano_cycle_get_32Test();
	if (rv != TC_PASS) {
		TC_ERROR("nano_cycle_get_32Test() failed\n");
		goto doneTests;
		}
#endif

doneTests:
	TC_END_RESULT(rv);
	TC_END_REPORT(rv);
}
