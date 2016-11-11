/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief Test semaphore APIs
 *
 * This modules tests the following semaphore routines:
 *
 * task_sem_group_reset()
 * task_sem_group_give()
 * task_sem_reset()
 * task_sem_give()
 * task_sem_count_get()
 * task_sem_take()
 * isr_sem_give()
 * fiber_sem_give()
 */

#define  __printf_like(f, a)

#include <zephyr.h>
#include <tc_util.h>

#include <util_test_common.h>

extern void trigger_isrSemaSignal(ksem_t semaphore);
extern void releaseTestFiber(void);

#define N_TESTS 10 /* number of tests to run */

#define OBJ_TIMEOUT  SECONDS(1)

extern ksem_t simpleSem;
extern ksem_t altSem;
extern ksem_t hpSem;
extern ksem_t manyBlockSem;
extern ksem_t group1Sem;
extern ksem_t group2Sem;
extern ksem_t group3Sem;
extern ksem_t group4Sem;
extern ksem_t blockHpSem;
extern ksem_t blockMpSem;
extern ksem_t blockLpSem;

extern ksem_t semList[];


/**
 *
 * @brief Signal semaphore that has no waiting tasks from ISR
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int simpleSemaTest(void)
{
	int  signalCount;
	int  i;
	int  status;

	/*
	 * Signal the semaphore several times from an ISR.  After each signal,
	 * check the signal count.
	 */

	for (i = 0; i < 5; i++) {
		trigger_isrSemaSignal(simpleSem);

		task_sleep(10);     /* Time for low priority task to run. */
		signalCount = task_sem_count_get(simpleSem);
		if (signalCount != (i + 1)) {
			TC_ERROR("<signalCount> error.  Expected %d, got %d\n",
					 i + 1, signalCount);
			return TC_FAIL;
		}
	}

	/*
	 * Signal the semaphore several times from a task.  After each signal,
	 * check the signal count.
	 */

	for (i = 5; i < 10; i++) {
		task_sem_give(simpleSem);

		signalCount = task_sem_count_get(simpleSem);
		if (signalCount != (i + 1)) {
			TC_ERROR("<signalCount> error.  Expected %d, got %d\n",
					 i + 1, signalCount);
			return TC_FAIL;
		}
	}

	/*
	 * Test the semaphore without wait.  Check the signal count after each
	 * attempt (it should be decrementing by 1 each time).
	 */

	for (i = 9; i >= 4; i--) {
		status = task_sem_take(simpleSem, TICKS_NONE);
		if (status != RC_OK) {
			TC_ERROR("task_sem_take(SIMPLE_SEM) error.  Expected %d, not %d.\n",
					 RC_OK, status);
			return TC_FAIL;
		}

		signalCount = task_sem_count_get(simpleSem);
		if (signalCount != i) {
			TC_ERROR("<signalCount> error.  Expected %d, not %d\n",
					 i, signalCount);
			return TC_FAIL;
		}
	}

	task_sem_reset(simpleSem);

	/*
	 * The semaphore's signal count should now be zero (0).  Test the
	 * semaphore without wait.  It should fail.
	 */

	for (i = 0; i < 10; i++) {
		status = task_sem_take(simpleSem, TICKS_NONE);
		if (status != RC_FAIL) {
			TC_ERROR("task_sem_take(SIMPLE_SEM) error.  Expected %d, got %d.\n",
					 RC_FAIL, status);
			return TC_FAIL;
		}

		signalCount = task_sem_count_get(simpleSem);
		if (signalCount != 0) {
			TC_ERROR("<signalCount> error.  Expected %d, not %d\n",
					 0, signalCount);
			return TC_FAIL;
		}
	}

	return TC_PASS;
}

/**
 *
 * @brief Test the waiting of a semaphore
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int simpleSemaWaitTest(void)
{
	int  status;
	int  i;

	for (i = 0; i < 5; i++) {
		/* Wait one second for SIMPLE_SEM.  Timeout is expected. */
		status = task_sem_take(simpleSem, OBJ_TIMEOUT);
		if (status != RC_TIME) {
			TC_ERROR("task_sem_take() error.  Expected %d, got %d\n",
					 RC_TIME, status);
			return TC_FAIL;
		}
	}

	/*
	 * Signal the semaphore upon which the alternate task is waiting.  The
	 * alternate task (which is at a lower priority) will cause SIMPLE_SEM
	 * to be signalled, thus waking this task.
	 */

	task_sem_give(altSem);

	status = task_sem_take(simpleSem, OBJ_TIMEOUT);
	if (status != RC_OK) {
		TC_ERROR("task_sem_take() error.  Expected %d, got %d\n",
				 RC_OK, status);
		return TC_FAIL;
	}

	/*
	 * Note that task_sem_take(TICKS_UNLIMITED) has been tested when waking up
	 * the alternate task.  Since previous tests had this task waiting, the
	 * alternate task must have had the time to enter the state where it is
	 * waiting for the ALTTASK_SEM semaphore to be given.  Thus, we do not need
	 * to test for it here.
	 *
	 * Now wait on SIMPLE_SEM again.  This time it will be woken up by an
	 * ISR signalling the semaphore.
	 */

	status = task_sem_take(simpleSem, OBJ_TIMEOUT);
	if (status != RC_OK) {
		TC_ERROR("task_sem_take() error.  Expected %d, got %d\n",
				 RC_OK, status);
		return TC_FAIL;
	}

	return TC_PASS;
}

/**
 *
 * @brief Test for a group of semaphores
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int simpleGroupTest(void)
{
	int     i;
	int     j;
	int     status;
	ksem_t  value;

	/* Ensure that the semaphores in the group are reset */

	task_sem_group_reset(semList);

	for (i = 0; semList[i] != ENDLIST; i++) {
		status = task_sem_count_get(semList[i]);
		if (status != 0) {
			TC_ERROR("task_sem_count_get() returned %d not %d\n", status, 0);
			return TC_FAIL;
		}
	}

	/* Timeout while waiting for a semaphore from the group */

	value = task_sem_group_take(semList, OBJ_TIMEOUT);
	if (value != ENDLIST) {
		TC_ERROR("task_sem_group_take() returned %d not %d\n",
				 value, ENDLIST);
		return TC_FAIL;
	}

	/* Signal the semaphores in the group */

	for (i = 0; i < 10; i++) {
		task_sem_group_give(semList);

		for (j = 0; semList[j] != ENDLIST; j++) {
			status = task_sem_count_get(semList[j]);
			if (status != i + 1) {
				TC_ERROR("task_sem_count_get() returned %d not %d\n",
						 status, i + 1);
				return TC_FAIL;
			}
		}
	}

	/* Get the semaphores */

	for (i = 9; i >= 5; i--) {
		value = task_sem_group_take(semList, 0);

		for (j = 0; semList[j] != ENDLIST; j++) {
			status = task_sem_count_get(semList[j]);
			if (status != (value == semList[j] ? i : 10)) {
				TC_ERROR("task_sem_count_get(0x%x) returned %d not %d\n",
						 semList[j], status, (value == semList[j]) ? i : 10);
				return TC_FAIL;
			}
		}
	}

	/* Reset the semaphores in the group */

	task_sem_group_reset(semList);

	for (i = 0; semList[i] != ENDLIST; i++) {
		status = task_sem_count_get(semList[i]);
		if (status != 0) {
			TC_ERROR("task_sem_count_get() returned %d not %d\n", status, 0);
			return TC_FAIL;
		}
	}

	return TC_PASS;
}

/**
 *
 * @brief Test a group of semaphores with waiting
 *
 * This routine tests the waiting feature on a group of semaphores.  Note that
 * timing out on a wait has already been tested so it need not be done again.
 *
 * @return TC_PASS on success, TC_FAIL on failure
 */

int simpleGroupWaitTest(void)
{
	int     i;
	ksem_t  sema;

	task_sem_give(altSem);    /* Wake the alternate task */

	/*
	 * Wait for a semaphore to be signalled by the alternate task.
	 * Each semaphore in the group will be tested.
	 */

	for (i = 0; semList[i] != ENDLIST; i++) {
		sema = task_sem_group_take(semList, TICKS_UNLIMITED);
		if (sema != semList[i]) {
			TC_ERROR("task_sem_group_take() error.  Expected %d, not %d\n",
					 (int) semList[i], (int) sema);
			return TC_FAIL;
		}
	}

	/*
	 * In the current implementation of semaphore groups, the taken
	 * semaphore is the first semaphore in 'semList'. Note that the
	 * semaphore taken is implementation-defined behavior and may
	 * change in the future.
	 */
	for (i = 0; i < 4; i++) {
		sema = task_sem_group_take(semList, TICKS_UNLIMITED);
		if (sema != semList[i]) {
			TC_ERROR("task_sem_group_take() error.  Expected %d, not %d\n",
					 (int) semList[i], (int) sema);
			return TC_FAIL;
		}
	}

	/*
	 * Again wait for a semaphore to be signalled.  This time, the alternate
	 * task will trigger an interrupt that signals the semaphore.
	 */

	for (i = 0; semList[i] != ENDLIST; i++) {
		sema = task_sem_group_take(semList, TICKS_UNLIMITED);
		if (sema != semList[i]) {
			TC_ERROR("task_sem_group_take() error.  Expected %d, not %d\n",
					 (int) semList[i], (int) sema);
			return TC_FAIL;
		}
	}

	return TC_PASS;
}

/**
 *
 * @brief Test semaphore signaling from fiber
 *
 * Routine starts a fiber and does the following tests:
 * - fiber signals the semaphore N times, task checks that task_sem_count_get is N
 * - task waits on a semaphore and fiber signals it
 * - task waits on a semaphore group and fiber signals each of them once. Task
 *   checks which of the semaphores has been signaled
 *
 * See also: testFiber.c
 *
 * @return TC_PASS on success or TC_FAIL on failure
 */
static int simpleFiberSemTest(void)
{
	int signalCount;
	int status;
	int i;
	ksem_t  sema;

	task_sem_reset(simpleSem);
	task_sem_group_reset(semList);

	/* let the fiber signal the semaphore and wait on it */
	releaseTestFiber();
	status = task_sem_take(simpleSem, OBJ_TIMEOUT);
	if (status != RC_OK) {
		TC_ERROR("task_sem_take() error.  Expected %d, got %d\n",
				 RC_OK, status);
		return TC_FAIL;
	}

	/* release the fiber and let it signal the semaphore N_TESTS times */
	releaseTestFiber();
	signalCount = task_sem_count_get(simpleSem);
	if (signalCount != N_TESTS) {
		TC_ERROR("<signalCount> error.  Expected %d, got %d\n",
				 N_TESTS, signalCount);
		return TC_FAIL;
	}

	/* wait on the semaphore group while the fiber signals each semaphore in it */
	for (i = 0; semList[i] != ENDLIST; i++) {
		releaseTestFiber();
		sema = task_sem_group_take(semList, OBJ_TIMEOUT);
		if (sema != semList[i]) {
			TC_ERROR("task_sem_group_take() error.  Expected %d, not %d\n",
					 (int) semList[i], (int) sema);
			return TC_FAIL;
		}
	}
	return TC_PASS;
}

/**
 *
 * @brief A high priority task
 *
 * @return TC_PASS or TC_FAIL
 */

int HighPriTask(void)
{
	int  status;

	/* Wait until task is activated */
	status = task_sem_take(hpSem, TICKS_UNLIMITED);
	if (status != RC_OK) {
		TC_ERROR("%s priority task failed to wait on %s: %d\n",
				 "High", "HIGH_PRI_SEM", status);
		return TC_FAIL;
	}

	/* Wait on a semaphore along with other tasks */
	status = task_sem_take(manyBlockSem, TICKS_UNLIMITED);
	if (status != RC_OK) {
		TC_ERROR("%s priority task failed to wait on %s: %d\n",
				 "High", "MANY_BLOCKED_SEM", status);
		return TC_FAIL;
	}

	/* Inform Regression test HP task is no longer blocked on MANY_BLOCKED_SEM*/
	task_sem_give(blockHpSem);

	return TC_PASS;

}

/**
 *
 * @brief A low priority task
 *
 * @return TC_PASS or TC_FAIL
 */

int LowPriTask(void)
{
	int  status;

	/* Wait on a semaphore along with other tasks */
	status = task_sem_take(manyBlockSem, TICKS_UNLIMITED);
	if (status != RC_OK) {
		TC_ERROR("%s priority task failed to wait on %s: %d\n",
				 "Low", "MANY_BLOCKED_SEM", status);
		return TC_FAIL;
	}

	/* Inform Regression test LP task is no longer blocked on MANY_BLOCKED_SEM*/
	task_sem_give(blockLpSem);

	return TC_PASS;
}

/**
 *
 * @brief Alternate task in the test suite
 *
 * This routine runs at a lower priority than RegressionTask().
 *
 * @return TC_PASS or TC_FAIL
 */

int AlternateTask(void)
{
	int  status;
	int  i;

	/* Wait until it is time to continue */
	status = task_sem_take(altSem, TICKS_UNLIMITED);
	if (status != RC_OK) {
		TC_ERROR("task_sem_take() error.  Expected %d, got %d\n",
				 RC_OK, status);
		return TC_FAIL;
	}

	/*
	 * After signalling the semaphore upon which the main (RegressionTask) task
	 * is waiting, control will pass back to the main (RegressionTask) task.
	 */

	task_sem_give(simpleSem);

	/*
	 * Control has returned to the alternate task.  Trigger an ISR that will
	 * signal the semaphore upon which RegressionTask is waiting.
	 */

	trigger_isrSemaSignal(simpleSem);

	/* Wait for RegressionTask to wake this task up */
	status = task_sem_take(altSem, TICKS_UNLIMITED);
	if (status != RC_OK) {
		TC_ERROR("task_sem_take() error.  Expected %d, got %d",
				 RC_OK, status);
		return TC_FAIL;
	}

	/* Wait on a semaphore that will have many waiters */
	status = task_sem_take(manyBlockSem, TICKS_UNLIMITED);
	if (status != RC_OK) {
		TC_ERROR("task_sem_take() error.  Expected %d, got %d",
				 RC_OK, status);
		return TC_FAIL;
	}

	/* Inform Regression test MP task is no longer blocked on MANY_BLOCKED_SEM*/
	task_sem_give(blockMpSem);

	/* Wait until the alternate task is needed again */
	status = task_sem_take(altSem, TICKS_UNLIMITED);
	if (status != RC_OK) {
		TC_ERROR("task_sem_take() error.  Expected %d, got %d",
				 RC_OK, status);
		return TC_FAIL;
	}

	for (i = 0; semList[i] != ENDLIST; i++) {
		task_sem_give(semList[i]);
		/* Context switch back to Regression Task */
	}

	task_sem_group_give(semList);
	/* Context switch back to Regression Task */

	for (i = 0; semList[i] != ENDLIST; i++) {
		trigger_isrSemaSignal(semList[i]);
		/* Context switch back to Regression Task */
	}

	return TC_PASS;
}

/**
 *
 * @brief Entry point to semaphore test suite
 *
 * This is the entry point to the semaphore test suite.
 *
 * @return TC_PASS or TC_FAIL
 */

int RegressionTask(void)
{
	int  tcRC;
	ksem_t value;
	ksem_t semBlockList[4];

	semBlockList[0] = blockHpSem;
	semBlockList[1] = blockMpSem;
	semBlockList[2] = blockLpSem;
	semBlockList[3] = ENDLIST;

	/* Signal a semaphore that has no waiting tasks. */
	TC_PRINT("Signal and test a semaphore without blocking\n");
	tcRC = simpleSemaTest();
	if (tcRC != TC_PASS) {
		return TC_FAIL;
	}

	/* Wait on a semaphore. */
	TC_PRINT("Signal and test a semaphore with blocking\n");
	tcRC = simpleSemaWaitTest();
	if (tcRC != TC_PASS) {
		return TC_FAIL;
	}

	/*
	 * Have many tasks wait on a semaphore (MANY_BLOCKED_SEM).  They will
	 * block in the following order:
	 *    LowPriorityTask    (low priority)
	 *    HighPriorityTask   (high priority)
	 *    AlternateTask      (medium priority)
	 *
	 * When the semaphore (MANY_BLOCKED_SEM) is signalled, HighPriorityTask
	 * will be woken first.
	 *
	 */

	TC_PRINT("Testing many tasks blocked on the same semaphore\n");

	task_sleep(OBJ_TIMEOUT);     /* Time for low priority task to run. */
	task_sem_give(hpSem);	   /* Wake high priority task */
	task_sem_give(altSem);	   /* Wake alternate task (medium priority) */
	task_sleep(OBJ_TIMEOUT);     /* Give alternate task time to run */

	task_sem_give(manyBlockSem);
	task_sleep(OBJ_TIMEOUT);     /* Ensure high priority task can run */

	value = task_sem_group_take(semBlockList, OBJ_TIMEOUT);
	if (value != blockHpSem) {
		TC_ERROR("%s priority task did not get semaphore: 0x%x\n",
				 "High", value);
		return TC_FAIL;
	}

	task_sem_give(manyBlockSem);
	task_sleep(OBJ_TIMEOUT);    /* Ensure medium priority task can run */
	value = task_sem_group_take(semBlockList, OBJ_TIMEOUT);
	if (value != blockMpSem) {
		TC_ERROR("%s priority task did not get semaphore: 0x%x\n",
				 "Medium", value);
		return TC_FAIL;
	}

	task_sem_give(manyBlockSem);

	task_sleep(OBJ_TIMEOUT);    /* Ensure low priority task can run */

	value = task_sem_group_take(semBlockList, OBJ_TIMEOUT);
	if (value != blockLpSem) {
		TC_ERROR("%s priority task did not get semaphore: 0x%x\n",
				 "Low", value);
		return TC_FAIL;
	}

	value = task_sem_group_take(semBlockList, OBJ_TIMEOUT);
	if (value != ENDLIST) {
		TC_ERROR("Group test Expecting ENDLIST, but got 0x%x\n",
				 value);
		return TC_FAIL;
	}


	TC_PRINT("Testing semaphore groups without blocking\n");
	tcRC = simpleGroupTest();
	if (tcRC != TC_PASS) {
		return TC_FAIL;
	}

	TC_PRINT("Testing semaphore groups with blocking\n");
	tcRC = simpleGroupWaitTest();
	if (tcRC != TC_PASS) {
		return TC_FAIL;
	}

	TC_PRINT("Testing semaphore release by fiber\n");
	tcRC = simpleFiberSemTest();
	if (tcRC != TC_PASS) {
		return TC_FAIL;
	}

	return TC_PASS;
}
