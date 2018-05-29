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
 * A thread that owns a mutex is promoted to the priority level of the
 * highest-priority thread attempting to lock the mutex.
 *
 * In addition, recursive locking capabilities and the use of a private mutex
 * are also tested.
 *
 * This module tests the following mutex routines:
 *
 *    k_mutex_lock
 *    k_mutex_unlock
 *
 * Timeline for priority inheritance testing:
 *   - 0.0  sec: thread_05, thread_06, thread_07, thread_08, thread_09, sleep
 *             : main thread takes mutex_1 then sleeps
 *   - 0.0  sec: thread_11 sleeps
 *   - 0.5  sec: thread_09 wakes and waits on mutex_1
 *   - 1.0  sec: main thread (@ priority 9) takes mutex_2 then sleeps
 *   - 1.5  sec: thread_08 wakes and waits on mutex_2
 *   - 2.0  sec: main thread (@ priority 8) takes mutex_3 then sleeps
 *   - 2.5  sec: thread_07 wakes and waits on mutex_3
 *   - 3.0  sec: main thread (@ priority 7) takes mutex_4 then sleeps
 *   - 3.5  sec: thread_05 wakes and waits on mutex_4
 *   - 3.5  sec: thread_11 wakes and waits on mutex_3
 *   - 3.75 sec: thread_06 wakes and waits on mutex_4
 *   - 4.0  sec: main thread wakes (@ priority 5) then sleeps
 *   - 4.5  sec: thread_05 times out
 *   - 5.0  sec: main thread wakes (@ priority 6) then gives mutex_4
 *             : main thread (@ priority 7) sleeps
 *   - 5.5  sec: thread_07 times out on mutex_3
 *   - 6.0  sec: main thread (@ priority 8) gives mutex_3
 *             : main thread (@ priority 8) gives mutex_2
 *             : main thread (@ priority 9) gives mutex_1
 *             : main thread (@ priority 10) sleeps
 */

#include <tc_util.h>
#include <zephyr.h>
#include <ztest.h>

#define STACKSIZE 512

static int tc_rc = TC_PASS;         /* test case return code */

K_MUTEX_DEFINE(private_mutex);


K_MUTEX_DEFINE(mutex_1);
K_MUTEX_DEFINE(mutex_2);
K_MUTEX_DEFINE(mutex_3);
K_MUTEX_DEFINE(mutex_4);

/**
 *
 * thread_05 -
 *
 * @return  N/A
 */

void thread_05(void)
{
	int rv;

	k_sleep(K_MSEC(3500));

	/* Wait and boost owner priority to 5 */
	rv = k_mutex_lock(&mutex_4, K_SECONDS(1));
	if (rv != -EAGAIN) {
		tc_rc = TC_FAIL;
		TC_ERROR("Failed to timeout on mutex 0x%x\n",
			 (u32_t)&mutex_4);
		return;
	}
}


/**
 *
 * thread_06 -
 *
 * @return  N/A
 */

void thread_06(void)
{
	int rv;

	k_sleep(K_MSEC(3750));

	/*
	 * Wait for the mutex.  There is a higher priority level thread waiting
	 * on the mutex, so request will not immediately contribute to raising
	 * the priority of the owning thread (main thread).  When thread_05
	 * times out this thread will become the highest priority waiting
	 * thread. The priority of the owning thread (main thread) will not
	 * drop back to 7, but will instead drop to 6.
	 */

	rv = k_mutex_lock(&mutex_4, K_SECONDS(2));
	if (rv != 0) {
		tc_rc = TC_FAIL;
		TC_ERROR("Failed to take mutex 0x%x\n", (u32_t)&mutex_4);
		return;
	}

	k_mutex_unlock(&mutex_4);
}

/**
 *
 * thread_07 -
 *
 * @return  N/A
 */

void thread_07(void)
{
	int rv;

	k_sleep(K_MSEC(2500));

	/*
	 * Wait and boost owner priority to 7.  While waiting, another thread of
	 * a very low priority level will also wait for the mutex.  thread_07 is
	 * expected to time out around the 5.5 second mark.  When it times out,
	 * thread_11 will become the only waiting thread for this mutex and the
	 * priority of the owning main thread will drop to 8.
	 */

	rv = k_mutex_lock(&mutex_3, K_SECONDS(3));
	if (rv != -EAGAIN) {
		tc_rc = TC_FAIL;
		TC_ERROR("Failed to timeout on mutex 0x%x\n",
			 (u32_t)&mutex_3);
		return;
	}

}

/**
 *
 * thread_08 -
 *
 * @return  N/A
 */

void thread_08(void)
{
	int rv;

	k_sleep(K_MSEC(1500));

	/* Wait and boost owner priority to 8 */
	rv = k_mutex_lock(&mutex_2, K_FOREVER);
	if (rv != 0) {
		tc_rc = TC_FAIL;
		TC_ERROR("Failed to take mutex 0x%x\n", (u32_t)&mutex_2);
		return;
	}

	k_mutex_unlock(&mutex_2);
}

/**
 *
 * thread_09 -
 *
 * @return  N/A
 */

void thread_09(void)
{
	int rv;

	k_sleep(K_MSEC(500));	/* Allow lower priority thread to run */

	/*<mutex_1> is already locked. */
	rv = k_mutex_lock(&mutex_1, K_NO_WAIT);
	if (rv != -EBUSY) {	/* This attempt to lock the mutex */
		/* should not succeed. */
		tc_rc = TC_FAIL;
		TC_ERROR("Failed to NOT take locked mutex 0x%x\n",
			 (u32_t)&mutex_1);
		return;
	}

	/* Wait and boost owner priority to 9 */
	rv = k_mutex_lock(&mutex_1, K_FOREVER);
	if (rv != 0) {
		tc_rc = TC_FAIL;
		TC_ERROR("Failed to take mutex 0x%x\n", (u32_t)&mutex_1);
		return;
	}

	k_mutex_unlock(&mutex_1);
}

/**
 *
 * thread_11 -
 *
 * @return N/A
 */

void thread_11(void)
{
	int rv;

	k_sleep(K_MSEC(3500));
	rv = k_mutex_lock(&mutex_3, K_FOREVER);
	if (rv != 0) {
		tc_rc = TC_FAIL;
		TC_ERROR("Failed to take mutex 0x%x\n", (u32_t)&mutex_2);
		return;
	}
	k_mutex_unlock(&mutex_3);
}

K_THREAD_STACK_DEFINE(thread_12_stack_area, STACKSIZE);
__kernel struct k_thread thread_12_thread_data;
extern void thread_12(void);

/**
 *
 * @brief Main thread to test thread_mutex_xxx interfaces
 *
 * This thread will lock on mutex_1, mutex_2, mutex_3 and mutex_4. It later
 * recursively locks private_mutex, releases it, then re-locks it.
 *
 * @return  N/A
 */

void test_mutex(void)
{
	/*
	 * Main thread(test_main) priority was 10 but ztest thread runs at
	 * priority -1. To run the test smoothly make both main and ztest
	 * threads run at same priority level.
	 */
	k_thread_priority_set(k_current_get(), 10);

	int rv;
	int i;
	struct k_mutex *mutexes[4] = { &mutex_1, &mutex_2, &mutex_3, &mutex_4 };
	struct k_mutex *givemutex[3] = { &mutex_3, &mutex_2, &mutex_1 };
	int priority[4] = { 9, 8, 7, 5 };
	int droppri[3] = { 8, 8, 9 };

	TC_START("Test kernel Mutex API");

	PRINT_LINE;

	/*
	 * 1st iteration: Take mutex_1; thread_09 waits on mutex_1
	 * 2nd iteration: Take mutex_2: thread_08 waits on mutex_2
	 * 3rd iteration: Take mutex_3; thread_07 waits on mutex_3
	 * 4th iteration: Take mutex_4; thread_05 waits on mutex_4
	 */

	for (i = 0; i < 4; i++) {
		rv = k_mutex_lock(mutexes[i], K_NO_WAIT);
		zassert_equal(rv, 0, "Failed to lock mutex 0x%x\n",
			      (u32_t)mutexes[i]);
		k_sleep(K_SECONDS(1));

		rv = k_thread_priority_get(k_current_get());
		zassert_equal(rv, priority[i], "expected priority %d, not %d\n",
			      priority[i], rv);

		/* Catch any errors from other threads */
		zassert_equal(tc_rc, TC_PASS, NULL);
	}

	/* ~ 4 seconds have passed */

	TC_PRINT("Done LOCKING!  Current priority = %d\n",
		 k_thread_priority_get(k_current_get()));

	k_sleep(K_SECONDS(1));       /* thread_05 should time out */

	/* ~ 5 seconds have passed */

	rv = k_thread_priority_get(k_current_get());
	zassert_equal(rv, 6, "%s timed out and out priority should drop.\n",
		      "thread_05");
	zassert_equal(rv, 6, "Expected priority %d, not %d\n", 6, rv);

	k_mutex_unlock(&mutex_4);
	rv = k_thread_priority_get(k_current_get());
	zassert_equal(rv, 7, "Gave %s and priority should drop.\n", "mutex_4");
	zassert_equal(rv, 7, "Expected priority %d, not %d\n", 7, rv);

	k_sleep(K_SECONDS(1));       /* thread_07 should time out */

	/* ~ 6 seconds have passed */

	for (i = 0; i < 3; i++) {
		rv = k_thread_priority_get(k_current_get());
		zassert_equal(rv, droppri[i], "Expected priority %d, not %d\n",
			      droppri[i], rv);
		k_mutex_unlock(givemutex[i]);

		zassert_equal(tc_rc, TC_PASS, NULL);
	}

	rv = k_thread_priority_get(k_current_get());
	zassert_equal(rv, 10, "Expected priority %d, not %d\n", 10, rv);

	k_sleep(K_SECONDS(1));     /* Give thread_11 time to run */

	zassert_equal(tc_rc, TC_PASS, NULL);

	/* test recursive locking using a private mutex */

	TC_PRINT("Testing recursive locking\n");

	rv = k_mutex_lock(&private_mutex, K_NO_WAIT);
	zassert_equal(rv, 0, "Failed to lock private mutex");

	rv = k_mutex_lock(&private_mutex, K_NO_WAIT);
	zassert_equal(rv, 0, "Failed to recursively lock private mutex");

	/* Start thread */
	k_thread_create(&thread_12_thread_data, thread_12_stack_area, STACKSIZE,
			(k_thread_entry_t)thread_12, NULL, NULL, NULL,
			K_PRIO_PREEMPT(12), K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);
	k_sleep(1);     /* Give thread_12 a chance to block on the mutex */

	k_mutex_unlock(&private_mutex);
	k_mutex_unlock(&private_mutex); /* thread_12 should now have lock */

	rv = k_mutex_lock(&private_mutex, K_NO_WAIT);
	zassert_equal(rv, -EBUSY, "Unexpectedly got lock on private mutex");

	rv = k_mutex_lock(&private_mutex, K_SECONDS(1));
	zassert_equal(rv, 0, "Failed to re-obtain lock on private mutex");

	k_mutex_unlock(&private_mutex);

	TC_PRINT("Recursive locking tests successful\n");
}

K_THREAD_DEFINE(THREAD_05, STACKSIZE, thread_05, NULL, NULL, NULL,
		5, K_USER, K_NO_WAIT);

K_THREAD_DEFINE(THREAD_06, STACKSIZE, thread_06, NULL, NULL, NULL,
		6, K_USER, K_NO_WAIT);

K_THREAD_DEFINE(THREAD_07, STACKSIZE, thread_07, NULL, NULL, NULL,
		7, K_USER, K_NO_WAIT);

K_THREAD_DEFINE(THREAD_08, STACKSIZE, thread_08, NULL, NULL, NULL,
		8, K_USER, K_NO_WAIT);

K_THREAD_DEFINE(THREAD_09, STACKSIZE, thread_09, NULL, NULL, NULL,
		9, K_USER, K_NO_WAIT);

K_THREAD_DEFINE(THREAD_11, STACKSIZE, thread_11, NULL, NULL, NULL,
		11, K_USER, K_NO_WAIT);

K_THREAD_ACCESS_GRANT(THREAD_05, &mutex_4);
K_THREAD_ACCESS_GRANT(THREAD_06, &mutex_4);
K_THREAD_ACCESS_GRANT(THREAD_07, &mutex_3);
K_THREAD_ACCESS_GRANT(THREAD_08, &mutex_2);
K_THREAD_ACCESS_GRANT(THREAD_09, &mutex_1);
K_THREAD_ACCESS_GRANT(THREAD_11, &mutex_3);

/*test case main entry*/
void test_main(void)
{
	k_thread_access_grant(k_current_get(), &private_mutex,
			      &mutex_1, &mutex_2, &mutex_3, &mutex_4,
			      &thread_12_thread_data, &thread_12_stack_area,
			      NULL);
	ztest_test_suite(mutex_complex, ztest_user_unit_test(test_mutex));
	ztest_run_test_suite(mutex_complex);
}
