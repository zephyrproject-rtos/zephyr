/* mutex.c - test microkernel mutex APIs */

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
This module demonstrates the microkernel's priority inheritance algorithm.
A task that owns a mutex is promoted to the priority level of the
highest-priority task attempting to lock the mutex.

This module tests the following mutex routines:

   task_mutex_lock, task_mutex_lock_wait, task_mutex_lock_wait_timeout
   task_mutex_unlock

Timeline
  - 0.0  sec: Task10, Task15, Task20, Task25, Task30, sleep
            : RegressionTask takes Mutex1 then sleeps
  - 0.0  sec: Task45 sleeps
  - 0.5  sec: Task30 wakes and waits on Mutex1
  - 1.0  sec: RegressionTask (@ priority 30) takes Mutex2 then sleeps
  - 1.5  sec: Task25 wakes and waits on Mutex2
  - 2.0  sec: RegressionTask (@ priority 25) takes Mutex3 then sleeps
  - 2.5  sec: Task20 wakes and waits on Mutex3
  - 3.0  sec: RegressionTask (@ priority 20) takes Mutex4 then sleeps
  - 3.5  sec: Task10 wakes and waits on Mutex4
  - 3.5  sec: Task45 wakes and waits on Mutex3
  - 3.75 sec: Task15 wakes and waits on Mutex4
  - 4.0  sec: RegressionTask wakes (@ priority 10) then sleeps
  - 4.5  sec: Task10 times out
  - 5.0  sec: RegressionTask wakes (@ priority 15) then gives Mutex4
            : RegressionTask (@ priority 20) sleeps
  - 5.5  sec: Task20 times out on Mutex3
  - 6.0  sec: RegressionTask (@ priority 25) gives Mutex3
            : RegressionTask (@ priority 25) gives Mutex2
            : RegressionTask (@ priority 30) gives Mutex1
            : RegressionTask (@ priority 40) sleeps
*/

#include <tc_util.h>
#include <zephyr.h>

#define  ONE_SECOND                 (sys_clock_ticks_per_sec)
#define  HALF_SECOND                (sys_clock_ticks_per_sec / 2)
#define  THIRD_SECOND               (sys_clock_ticks_per_sec / 3)
#define  FOURTH_SECOND              (sys_clock_ticks_per_sec / 4)

static int tcRC = TC_PASS;         /* test case return code */

/*******************************************************************************
*
* Task10 -
*
* RETURNS:  N/A
*/

void Task10(void)
{
	int  rv;

	task_sleep(3 * ONE_SECOND + HALF_SECOND);

	/* Wait and boost owner priority to 10 */
	rv = task_mutex_lock_wait_timeout(Mutex4, ONE_SECOND);
	if (rv != RC_TIME) {
		tcRC = TC_FAIL;
		TC_ERROR("Failed to timeout on mutex 0x%x\n", Mutex4);
		return;
	}
}   /* Task10 */


/*******************************************************************************
*
* Task15 -
*
* RETURNS:  N/A
*/

void Task15(void)
{
	int  rv;

	task_sleep(3 * ONE_SECOND + 3 * FOURTH_SECOND);

	/*
	 * Wait for the mutex.  There is a higher priority level task waiting
	 * on the mutex, so request will not immediately contribute to raising
	 * the priority of the owning task (RegressionTask).  When Task10 times out
	 * this task will become the highest priority waiting task.  The priority
	 * of the owning task (RegressionTask) will not drop back to 20, but will
	 * instead drop to 15.
	 */

	rv = task_mutex_lock_wait_timeout(Mutex4, 2 * ONE_SECOND);
	if (rv != RC_OK) {
		tcRC = TC_FAIL;
		TC_ERROR("Failed to take mutex 0x%x\n", Mutex4);
		return;
	}

	task_mutex_unlock(Mutex4);
}

/*******************************************************************************
*
* Task20 -
*
* RETURNS:  N/A
*/

void Task20(void)
{
	int  rv;

	task_sleep(2 * ONE_SECOND + HALF_SECOND);

	/*
	 * Wait and boost owner priority to 20.  While waiting, another task of
	 * a very low priority level will also wait for the mutex.  Task20 is
	 * expected to time out around the 5.5 second mark.  When it times out,
	 * Task45 will become the only waiting task for this mutex and the
	 * priority of the owning task RegressionTask will drop to 25.
	 */

	rv = task_mutex_lock_wait_timeout(Mutex3, 3 * ONE_SECOND);
	if (rv != RC_TIME) {
		tcRC = TC_FAIL;
		TC_ERROR("Failed to timeout on mutex 0x%x\n", Mutex3);
		return;
	}

}   /* Task20 */

/*******************************************************************************
*
* Task25 -
*
* RETURNS:  N/A
*/

void Task25(void)
{
	int  rv;

	task_sleep(ONE_SECOND + HALF_SECOND);

	rv = task_mutex_lock_wait(Mutex2);      /* Wait and boost owner priority to 25 */
	if (rv != RC_OK) {
		tcRC = TC_FAIL;
		TC_ERROR("Failed to take mutex 0x%x\n", Mutex2);
		return;
	}

	task_mutex_unlock(Mutex2);
}   /* Task25 */

/*******************************************************************************
*
* Task30 -
*
* RETURNS:  N/A
*/

void Task30(void)
{
	int  rv;

	task_sleep(HALF_SECOND);   /* Allow lower priority task to run */

	rv = task_mutex_lock(Mutex1);       /* <Mutex1> is already locked. */
	if (rv != RC_FAIL) {            /* This attempt to lock the mutex */
		/* should not succeed. */
		tcRC = TC_FAIL;
		TC_ERROR("Failed to NOT take locked mutex 0x%x\n", Mutex1);
		return;
	}

	rv = task_mutex_lock_wait(Mutex1);      /* Wait and boost owner priority to 30 */
	if (rv != RC_OK) {
		tcRC = TC_FAIL;
		TC_ERROR("Failed to take mutex 0x%x\n", Mutex1);
		return;
	}

	task_mutex_unlock(Mutex1);
}

/*******************************************************************************
*
* Task45 -
*
* RETURNS: N/A
*/

void Task45(void)
{
	int rv;

	task_sleep(3 * ONE_SECOND + HALF_SECOND);
	rv = task_mutex_lock_wait(Mutex3);
	if (rv != RC_OK) {
		tcRC = TC_FAIL;
		TC_ERROR("Failed to take mutex 0x%x\n", Mutex2);
		return;
	}
	task_mutex_unlock(Mutex3);
}

/*******************************************************************************
*
* RegressionTask - main task to test task_mutex_xxx interfaces
*
* This task will lock on Mutex1, Mutex2, Mutex3 and Mutex4.
*
* RETURNS:  N/A
*/

void RegressionTask(void)
{
	int    rv;
	int    i;
	kmutex_t  mutexes[4] = {Mutex1, Mutex2, Mutex3, Mutex4};
	kmutex_t  giveMutex[3] = {Mutex3, Mutex2, Mutex1};
	int    priority[4] = {30, 25, 20, 10};
	int    dropPri[3] = {25, 25, 30};

	TC_START("Test Microkernel Mutex API");

	PRINT_LINE;

	/*
	 * 1st iteration: Take Mutex1; Task30 waits on Mutex1
	 * 2nd iteration: Take Mutex2: Task25 waits on Mutex2
	 * 3rd iteration: Take Mutex3; Task20 waits on Mutex3
	 * 4th iteration: Take Mutex4; Task10 waits on Mutex4
	 */

	for (i = 0; i < 4; i++) {
		rv = task_mutex_lock(mutexes[i]);
		if (rv != RC_OK) {
			TC_ERROR("Failed to lock mutex 0x%x\n", mutexes[i]);
			tcRC = TC_FAIL;
			goto errorReturn;
		}
		task_sleep(ONE_SECOND);

		rv = task_priority_get();
		if (rv != priority[i]) {
			TC_ERROR("Expected priority %d, not %d\n", priority[i], rv);
			tcRC = TC_FAIL;
			goto errorReturn;
		}

		if (tcRC != TC_PASS) {     /* Catch any errors from other tasks */
			goto errorReturn;
		}
	}

	/* ~ 4 seconds have passed */

	TC_PRINT("Done LOCKING!  Current priority = %d\n", task_priority_get());

	task_sleep(ONE_SECOND);       /* Task10 should time out */

	/* ~ 5 seconds have passed */

	rv = task_priority_get();
	if (rv != 15) {
		TC_ERROR("%s timed out and out priority should drop.\n", "Task10");
		TC_ERROR("Expected priority %d, not %d\n", 15, rv);
		tcRC = TC_FAIL;
		goto errorReturn;
	}

	task_mutex_unlock(Mutex4);
	rv = task_priority_get();
	if (rv != 20) {
		TC_ERROR("Gave %s and priority should drop.\n", "Mutex4");
		TC_ERROR("Expected priority %d, not %d\n", 20, rv);
		tcRC = TC_FAIL;
		goto errorReturn;
	}

	task_sleep(ONE_SECOND);       /* Task20 should time out */

	/* ~ 6 seconds have passed */

	for (i = 0; i < 3; i++) {
		rv = task_priority_get();
		if (rv != dropPri[i]) {
			TC_ERROR("Expected priority %d, not %d\n", dropPri[i], rv);
			tcRC = TC_FAIL;
			goto errorReturn;
		}
		task_mutex_unlock(giveMutex[i]);

		if (tcRC != TC_PASS) {
			goto errorReturn;
		}
	}

	rv = task_priority_get();
	if (rv != 40) {
		TC_ERROR("Expected priority %d, not %d\n", 40, rv);
		tcRC = TC_FAIL;
		goto errorReturn;
	}

	task_sleep(ONE_SECOND);     /* Give Task45 time to run */

	if (tcRC != TC_PASS) {
		goto errorReturn;
	}

errorReturn:
	TC_END_RESULT(tcRC);
	TC_END_REPORT(tcRC);
}  /* RegressionTask */
