/*
 * Copyright (c) 2012-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test kernel mutex APIs
 *
 *
 * This module demonstrates the kernel's priority inheritance algorithm.
 * A task that owns a mutex is promoted to the priority level of the
 * highest-priority task attempting to lock the mutex.
 *
 * In addition, recusive locking capabilities and the use of a private mutex
 * are also tested.
 *
 * This module tests the following mutex routines:
 *
 *    task_mutex_lock
 *    task_mutex_unlock
 *    task_mutex_init
 *
 * Timeline for priority inheritance testing:
 *   - 0.0  sec: Task05, Task06, Task07, Task08, Task09, sleep
 *             : RegressionTask takes Mutex1 then sleeps
 *   - 0.0  sec: Task11 sleeps
 *   - 0.5  sec: Task09 wakes and waits on Mutex1
 *   - 1.0  sec: RegressionTask (@ priority 9) takes Mutex2 then sleeps
 *   - 1.5  sec: Task08 wakes and waits on Mutex2
 *   - 2.0  sec: RegressionTask (@ priority 8) takes Mutex3 then sleeps
 *   - 2.5  sec: Task07 wakes and waits on Mutex3
 *   - 3.0  sec: RegressionTask (@ priority 7) takes Mutex4 then sleeps
 *   - 3.5  sec: Task05 wakes and waits on Mutex4
 *   - 3.5  sec: Task11 wakes and waits on Mutex3
 *   - 3.75 sec: Task06 wakes and waits on Mutex4
 *   - 4.0  sec: RegressionTask wakes (@ priority 5) then sleeps
 *   - 4.5  sec: Task05 times out
 *   - 5.0  sec: RegressionTask wakes (@ priority 6) then gives Mutex4
 *             : RegressionTask (@ priority 7) sleeps
 *   - 5.5  sec: Task07 times out on Mutex3
 *   - 6.0  sec: RegressionTask (@ priority 8) gives Mutex3
 *             : RegressionTask (@ priority 8) gives Mutex2
 *             : RegressionTask (@ priority 9) gives Mutex1
 *             : RegressionTask (@ priority 10) sleeps
 */

#include <tc_util.h>
#include <zephyr.h>

#define STACKSIZE 512

static int tcRC = TC_PASS;         /* test case return code */

K_MUTEX_DEFINE(private_mutex);


K_MUTEX_DEFINE(Mutex1);
K_MUTEX_DEFINE(Mutex2);
K_MUTEX_DEFINE(Mutex3);
K_MUTEX_DEFINE(Mutex4);

/**
 *
 * Task05 -
 *
 * @return  N/A
 */

void Task05(void)
{
	int  rv;

	k_sleep(K_MSEC(3500));

	/* Wait and boost owner priority to 5 */
	rv = k_mutex_lock(&Mutex4, K_SECONDS(1));
	if (rv != -EAGAIN) {
		tcRC = TC_FAIL;
		TC_ERROR("Failed to timeout on mutex 0x%x\n",
				 (u32_t)&Mutex4);
		return;
	}
}


/**
 *
 * Task06 -
 *
 * @return  N/A
 */

void Task06(void)
{
	int  rv;

	k_sleep(K_MSEC(3750));

	/*
	 * Wait for the mutex.  There is a higher priority level task waiting
	 * on the mutex, so request will not immediately contribute to raising
	 * the priority of the owning task (RegressionTask).  When Task05
	 * times out this task will become the highest priority waiting task.
	 * The priority of the owning task (RegressionTask) will not drop back
	 * to 7, but will instead drop to 6.
	 */

	rv = k_mutex_lock(&Mutex4, K_SECONDS(2));
	if (rv != 0) {
		tcRC = TC_FAIL;
		TC_ERROR("Failed to take mutex 0x%x\n", (u32_t)&Mutex4);
		return;
	}

	k_mutex_unlock(&Mutex4);
}

/**
 *
 * Task07 -
 *
 * @return  N/A
 */

void Task07(void)
{
	int  rv;

	k_sleep(K_MSEC(2500));

	/*
	 * Wait and boost owner priority to 7.  While waiting, another task of
	 * a very low priority level will also wait for the mutex.  Task07 is
	 * expected to time out around the 5.5 second mark.  When it times out,
	 * Task11 will become the only waiting task for this mutex and the
	 * priority of the owning task RegressionTask will drop to 8.
	 */

	rv = k_mutex_lock(&Mutex3, K_SECONDS(3));
	if (rv != -EAGAIN) {
		tcRC = TC_FAIL;
		TC_ERROR("Failed to timeout on mutex 0x%x\n",
			 (u32_t)&Mutex3);
		return;
	}

}

/**
 *
 * Task08 -
 *
 * @return  N/A
 */

void Task08(void)
{
	int  rv;

	k_sleep(K_MSEC(1500));

	/* Wait and boost owner priority to 8 */
	rv = k_mutex_lock(&Mutex2, K_FOREVER);
	if (rv != 0) {
		tcRC = TC_FAIL;
		TC_ERROR("Failed to take mutex 0x%x\n", (u32_t)&Mutex2);
		return;
	}

	k_mutex_unlock(&Mutex2);
}

/**
 *
 * Task09 -
 *
 * @return  N/A
 */

void Task09(void)
{
	int  rv;

	k_sleep(K_MSEC(500));   /* Allow lower priority task to run */

	rv = k_mutex_lock(&Mutex1, K_NO_WAIT); /*<Mutex1> is already locked. */
	if (rv != -EBUSY) {            /* This attempt to lock the mutex */
		/* should not succeed. */
		tcRC = TC_FAIL;
		TC_ERROR("Failed to NOT take locked mutex 0x%x\n",
			 (u32_t)&Mutex1);
		return;
	}

	/* Wait and boost owner priority to 9 */
	rv = k_mutex_lock(&Mutex1, K_FOREVER);
	if (rv != 0) {
		tcRC = TC_FAIL;
		TC_ERROR("Failed to take mutex 0x%x\n", (u32_t)&Mutex1);
		return;
	}

	k_mutex_unlock(&Mutex1);
}

/**
 *
 * Task11 -
 *
 * @return N/A
 */

void Task11(void)
{
	int rv;

	k_sleep(K_MSEC(3500));
	rv = k_mutex_lock(&Mutex3, K_FOREVER);
	if (rv != 0) {
		tcRC = TC_FAIL;
		TC_ERROR("Failed to take mutex 0x%x\n", (u32_t)&Mutex2);
		return;
	}
	k_mutex_unlock(&Mutex3);
}

K_THREAD_STACK_DEFINE(task12_stack_area, STACKSIZE);
struct k_thread task12_thread_data;
extern void Task12(void);

/**
 *
 * @brief Main task to test task_mutex_xxx interfaces
 *
 * This task will lock on Mutex1, Mutex2, Mutex3 and Mutex4. It later
 * recursively locks private_mutex, releases it, then re-locks it.
 *
 * @return  N/A
 */

void RegressionTask(void)
{
	int    rv;
	int    i;
	struct k_mutex *mutexes[4] = {&Mutex1, &Mutex2, &Mutex3, &Mutex4};
	struct k_mutex *giveMutex[3] = {&Mutex3, &Mutex2, &Mutex1};
	int    priority[4] = {9, 8, 7, 5};
	int    dropPri[3] = {8, 8, 9};

	TC_START("Test kernel Mutex API");

	PRINT_LINE;

	/*
	 * 1st iteration: Take Mutex1; Task09 waits on Mutex1
	 * 2nd iteration: Take Mutex2: Task08 waits on Mutex2
	 * 3rd iteration: Take Mutex3; Task07 waits on Mutex3
	 * 4th iteration: Take Mutex4; Task05 waits on Mutex4
	 */

	for (i = 0; i < 4; i++) {
		rv = k_mutex_lock(mutexes[i], K_NO_WAIT);
		if (rv != 0) {
			TC_ERROR("Failed to lock mutex 0x%x\n",
				 (u32_t)mutexes[i]);
			tcRC = TC_FAIL;
			goto errorReturn;
		}
		k_sleep(K_SECONDS(1));

		rv = k_thread_priority_get(k_current_get());
		if (rv != priority[i]) {
			TC_ERROR("Expected priority %d, not %d\n",
				 priority[i], rv);
			tcRC = TC_FAIL;
			goto errorReturn;
		}

		if (tcRC != TC_PASS) {  /* Catch any errors from other tasks */
			goto errorReturn;
		}
	}

	/* ~ 4 seconds have passed */

	TC_PRINT("Done LOCKING!  Current priority = %d\n",
			k_thread_priority_get(k_current_get()));

	k_sleep(K_SECONDS(1));       /* Task05 should time out */

	/* ~ 5 seconds have passed */

	rv = k_thread_priority_get(k_current_get());
	if (rv != 6) {
		TC_ERROR("%s timed out and out priority should drop.\n",
			 "Task05");
		TC_ERROR("Expected priority %d, not %d\n", 6, rv);
		tcRC = TC_FAIL;
		goto errorReturn;
	}

	k_mutex_unlock(&Mutex4);
	rv = k_thread_priority_get(k_current_get());
	if (rv != 7) {
		TC_ERROR("Gave %s and priority should drop.\n", "Mutex4");
		TC_ERROR("Expected priority %d, not %d\n", 7, rv);
		tcRC = TC_FAIL;
		goto errorReturn;
	}

	k_sleep(K_SECONDS(1));       /* Task07 should time out */

	/* ~ 6 seconds have passed */

	for (i = 0; i < 3; i++) {
		rv = k_thread_priority_get(k_current_get());
		if (rv != dropPri[i]) {
			TC_ERROR("Expected priority %d, not %d\n",
					 dropPri[i], rv);
			tcRC = TC_FAIL;
			goto errorReturn;
		}
		k_mutex_unlock(giveMutex[i]);

		if (tcRC != TC_PASS) {
			goto errorReturn;
		}
	}

	rv = k_thread_priority_get(k_current_get());
	if (rv != 10) {
		TC_ERROR("Expected priority %d, not %d\n", 10, rv);
		tcRC = TC_FAIL;
		goto errorReturn;
	}

	k_sleep(K_SECONDS(1));     /* Give Task11 time to run */

	if (tcRC != TC_PASS) {
		goto errorReturn;
	}

	/* test recursive locking using a private mutex */

	TC_PRINT("Testing recursive locking\n");

	rv = k_mutex_lock(&private_mutex, K_NO_WAIT);
	if (rv != 0) {
		TC_ERROR("Failed to lock private mutex\n");
		tcRC = TC_FAIL;
		goto errorReturn;
	}

	rv = k_mutex_lock(&private_mutex, K_NO_WAIT);
	if (rv != 0) {
		TC_ERROR("Failed to recursively lock private mutex\n");
		tcRC = TC_FAIL;
		goto errorReturn;
	}

		/* Start thread */
	k_thread_create(&task12_thread_data, task12_stack_area, STACKSIZE,
			(k_thread_entry_t)Task12, NULL, NULL, NULL,
			K_PRIO_PREEMPT(12), 0, K_NO_WAIT);
	k_sleep(1);	/* Give Task12 a chance to block on the mutex */

	k_mutex_unlock(&private_mutex);
	k_mutex_unlock(&private_mutex); /* Task12 should now have lock */

	rv = k_mutex_lock(&private_mutex, K_NO_WAIT);
	if (rv != -EBUSY) {
		TC_ERROR("Unexpectedly got lock on private mutex\n");
		tcRC = TC_FAIL;
		goto errorReturn;
	}

	rv = k_mutex_lock(&private_mutex, K_SECONDS(1));
	if (rv != 0) {
		TC_ERROR("Failed to re-obtain lock on private mutex\n");
		tcRC = TC_FAIL;
		goto errorReturn;
	}

	k_mutex_unlock(&private_mutex);

	TC_PRINT("Recursive locking tests successful\n");

errorReturn:
	TC_END_RESULT(tcRC);
	TC_END_REPORT(tcRC);
}  /* RegressionTask */


K_THREAD_DEFINE(TASK05, STACKSIZE, Task05, NULL, NULL, NULL,
			5, 0, K_NO_WAIT);

K_THREAD_DEFINE(TASK06, STACKSIZE, Task06, NULL, NULL, NULL,
			6, 0, K_NO_WAIT);

K_THREAD_DEFINE(TASK07, STACKSIZE, Task07, NULL, NULL, NULL,
			7, 0, K_NO_WAIT);

K_THREAD_DEFINE(TASK08, STACKSIZE, Task08, NULL, NULL, NULL,
			8, 0, K_NO_WAIT);

K_THREAD_DEFINE(TASK09, STACKSIZE, Task09, NULL, NULL, NULL,
			9, 0, K_NO_WAIT);

K_THREAD_DEFINE(TASK11, STACKSIZE, Task11, NULL, NULL, NULL,
			11, 0, K_NO_WAIT);

K_THREAD_DEFINE(REGRESSTASK, STACKSIZE, RegressionTask, NULL, NULL, NULL,
			10, 0, K_NO_WAIT);
