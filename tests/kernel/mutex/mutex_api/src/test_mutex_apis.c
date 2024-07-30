/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>

#define TIMEOUT 500
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define THREAD_HIGH_PRIORITY 1
#define THREAD_MID_PRIORITY 3
#define THREAD_LOW_PRIORITY 5

/* use to pass case type to threads */
static ZTEST_DMEM int case_type;
static ZTEST_DMEM int thread_ret = TC_FAIL;

/**TESTPOINT: init via K_MUTEX_DEFINE*/
K_MUTEX_DEFINE(kmutex);
static struct k_mutex tmutex;

static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static K_THREAD_STACK_DEFINE(tstack2, STACK_SIZE);
static K_THREAD_STACK_DEFINE(tstack3, STACK_SIZE);
static struct k_thread tdata;
static struct k_thread tdata2;
static struct k_thread tdata3;

static void tThread_entry_lock_forever(void *p1, void *p2, void *p3)
{
	zassert_false(k_mutex_lock((struct k_mutex *)p1, K_FOREVER) == 0,
		      "access locked resource from spawn thread");
	/* should not hit here */
}

static void tThread_entry_lock_no_wait(void *p1, void *p2, void *p3)
{
	zassert_true(k_mutex_lock((struct k_mutex *)p1, K_NO_WAIT) != 0);
	TC_PRINT("bypass locked resource from spawn thread\n");
}

static void tThread_entry_lock_timeout_fail(void *p1, void *p2, void *p3)
{
	zassert_true(k_mutex_lock((struct k_mutex *)p1,
				  K_MSEC(TIMEOUT - 100)) != 0, NULL);
	TC_PRINT("bypass locked resource from spawn thread\n");
}

static void tThread_entry_lock_timeout_pass(void *p1, void *p2, void *p3)
{
	zassert_true(k_mutex_lock((struct k_mutex *)p1,
				  K_MSEC(TIMEOUT + 100)) == 0, NULL);
	TC_PRINT("access resource from spawn thread\n");
	k_mutex_unlock((struct k_mutex *)p1);
}

static void tmutex_test_lock(struct k_mutex *pmutex,
			     void (*entry_fn)(void *, void *, void *))
{
	k_mutex_init(pmutex);
	k_thread_create(&tdata, tstack, STACK_SIZE,
			entry_fn, pmutex, NULL, NULL,
			K_PRIO_PREEMPT(0),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	zassert_true(k_mutex_lock(pmutex, K_FOREVER) == 0);
	TC_PRINT("access resource from main thread\n");

	/* wait for spawn thread to take action */
	k_msleep(TIMEOUT);
}

static void tmutex_test_lock_timeout(struct k_mutex *pmutex,
				     void (*entry_fn)(void *, void *, void *))
{
	/**TESTPOINT: test k_mutex_init mutex*/
	k_mutex_init(pmutex);
	k_thread_create(&tdata, tstack, STACK_SIZE,
			entry_fn, pmutex, NULL, NULL,
			K_PRIO_PREEMPT(0),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	zassert_true(k_mutex_lock(pmutex, K_FOREVER) == 0);
	TC_PRINT("access resource from main thread\n");

	/* wait for spawn thread to take action */
	k_msleep(TIMEOUT);
	k_mutex_unlock(pmutex);
	k_msleep(TIMEOUT);

}

static void tmutex_test_lock_unlock(struct k_mutex *pmutex)
{
	k_mutex_init(pmutex);
	zassert_true(k_mutex_lock(pmutex, K_FOREVER) == 0,
		     "fail to lock K_FOREVER");
	k_mutex_unlock(pmutex);
	zassert_true(k_mutex_lock(pmutex, K_NO_WAIT) == 0,
		     "fail to lock K_NO_WAIT");
	k_mutex_unlock(pmutex);
	zassert_true(k_mutex_lock(pmutex, K_MSEC(TIMEOUT)) == 0,
		     "fail to lock TIMEOUT");
	k_mutex_unlock(pmutex);
}

static void tThread_T1_priority_inheritance(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p3);

	/* t1 will get mutex first */
	zassert_true(k_mutex_lock((struct k_mutex *)p1, K_FOREVER) == 0,
		      "access locked resource from spawn thread T1");

	/* record its original priority */
	int priority_origin = k_thread_priority_get((k_tid_t)p2);

	/* wait for a time period to see if priority inheritance happened */
	k_sleep(K_MSEC(500));

	int priority = k_thread_priority_get((k_tid_t)p2);

	if (case_type == 1) {
		zassert_equal(priority, THREAD_HIGH_PRIORITY,
			"priority inheritance not happened!");

		k_mutex_unlock((struct k_mutex *)p1);

		/* check if priority set back to original one */
		priority = k_thread_priority_get((k_tid_t)p2);

		zassert_equal(priority, priority_origin,
			"priority inheritance adjust back not happened!");
	} else if (case_type == 2) {
		zassert_equal(priority, priority_origin,
			"priority inheritance should not be happened!");

		/* wait for t2 timeout to get mutex*/
		k_sleep(K_MSEC(TIMEOUT));

		k_mutex_unlock((struct k_mutex *)p1);
	} else if (case_type == 3) {
		zassert_equal(priority, THREAD_HIGH_PRIORITY,
			"priority inheritance not happened!");

		/* wait for t2 timeout to get mutex*/
		k_sleep(K_MSEC(TIMEOUT));

		k_mutex_unlock((struct k_mutex *)p1);
	} else {
		zassert_true(0, "should not be here!");
	}
}

static void tThread_T2_priority_inheritance(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	if (case_type == 1) {
		zassert_true(k_mutex_lock((struct k_mutex *)p1, K_FOREVER) == 0,
		      "access locked resource from spawn thread T2");

		k_mutex_unlock((struct k_mutex *)p1);
	} else if (case_type == 2 || case_type == 3) {
		zassert_false(k_mutex_lock((struct k_mutex *)p1,
				K_MSEC(100)) == 0,
				"T2 should not get the resource");
	} else {
		zassert_true(0, "should not be here!");
	}
}

static void tThread_lock_with_time_period(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	zassert_true(k_mutex_lock((struct k_mutex *)p1, K_FOREVER) == 0,
		      "access locked resource from spawn thread");

	/* This thread will hold mutex for 600 ms, then release it */
	k_sleep(K_MSEC(TIMEOUT + 100));

	k_mutex_unlock((struct k_mutex *)p1);
}

static void tThread_waiter(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* This thread participates in recursive locking tests */
	/* Wait for mutex to be released */
	zassert_true(k_mutex_lock((struct k_mutex *)p1, K_FOREVER) == 0,
			"Failed to get the test_mutex");

	/* keep the next waiter waiting for a while */
	thread_ret = TC_PASS;
	k_mutex_unlock((struct k_mutex *)p1);
}

/*test cases*/
ZTEST_USER(mutex_api_1cpu, test_mutex_reent_lock_forever)
{
	/**TESTPOINT: test k_mutex_init mutex*/
	k_mutex_init(&tmutex);
	tmutex_test_lock(&tmutex, tThread_entry_lock_forever);
	k_thread_abort(&tdata);

	/**TESTPOINT: test K_MUTEX_DEFINE mutex*/
	tmutex_test_lock(&kmutex, tThread_entry_lock_forever);
	k_thread_abort(&tdata);
}

ZTEST_USER(mutex_api, test_mutex_reent_lock_no_wait)
{
	/**TESTPOINT: test k_mutex_init mutex*/
	tmutex_test_lock(&tmutex, tThread_entry_lock_no_wait);

	/**TESTPOINT: test K_MUTEX_DEFINE mutex*/
	tmutex_test_lock(&kmutex, tThread_entry_lock_no_wait);
}

ZTEST_USER(mutex_api, test_mutex_reent_lock_timeout_fail)
{
	/**TESTPOINT: test k_mutex_init mutex*/
	tmutex_test_lock_timeout(&tmutex, tThread_entry_lock_timeout_fail);

	/**TESTPOINT: test K_MUTEX_DEFINE mutex*/
	tmutex_test_lock_timeout(&kmutex, tThread_entry_lock_no_wait);
}

ZTEST_USER(mutex_api_1cpu, test_mutex_reent_lock_timeout_pass)
{
	/**TESTPOINT: test k_mutex_init mutex*/
	tmutex_test_lock_timeout(&tmutex, tThread_entry_lock_timeout_pass);

	/**TESTPOINT: test K_MUTEX_DEFINE mutex*/
	tmutex_test_lock_timeout(&kmutex, tThread_entry_lock_no_wait);
}

ZTEST_USER(mutex_api_1cpu, test_mutex_lock_unlock)
{
	/**TESTPOINT: test k_mutex_init mutex*/
	tmutex_test_lock_unlock(&tmutex);

	/**TESTPOINT: test K_MUTEX_DEFINE mutex*/
	tmutex_test_lock_unlock(&kmutex);
}

/**
 * @brief Test recursive mutex
 * @details To verify that getting a lock of a mutex already locked will
 * succeed and waiters will be unblocked only when the number of locks
 * reaches zero.
 * @ingroup kernel_mutex_tests
 */
ZTEST_USER(mutex_api, test_mutex_recursive)
{
	k_mutex_init(&tmutex);

	/**TESTPOINT: when mutex has no owner, we cannot unlock it */
	zassert_true(k_mutex_unlock(&tmutex) == -EINVAL,
			"fail: mutex has no owner");

	zassert_true(k_mutex_lock(&tmutex, K_NO_WAIT) == 0,
			"Failed to lock mutex");

	/**TESTPOINT: lock the mutex recursively */
	zassert_true(k_mutex_lock(&tmutex, K_NO_WAIT) == 0,
		"Failed to recursively lock mutex");

	thread_ret = TC_FAIL;
	/* Spawn a waiter thread */
	k_thread_create(&tdata3, tstack3, STACK_SIZE,
			tThread_waiter, &tmutex, NULL, NULL,
			K_PRIO_PREEMPT(12),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	zassert_true(thread_ret == TC_FAIL,
		"waiter thread should block on the recursively locked mutex");

	zassert_true(k_mutex_unlock(&tmutex) == 0, "fail to unlock");

	/**TESTPOINT: unlock the mutex recursively */
	zassert_true(thread_ret == TC_FAIL,
		"waiter thread should still block on the locked mutex");

	zassert_true(k_mutex_unlock(&tmutex) == 0, "fail to unlock");

	/* Give thread_waiter a chance to get the mutex */
	k_sleep(K_MSEC(1));

	/**TESTPOINT: waiter thread got the mutex */
	zassert_true(thread_ret == TC_PASS,
			"waiter thread can't take the mutex");
}

/**
 * @brief Test mutex's priority inheritance mechanism
 * @details To verify mutex provide priority inheritance to prevent priority
 * inversion, and there are 3 cases need to run.
 * The thread T1 hold the mutex first and cases list as below:
 * - case 1. When priority T2 > T1, priority inheritance happened.
 * - case 2. When priority T1 > T2, priority inheritance won't happened.
 * - case 3. When priority T2 > T3 > T1, priority inheritance happened but T2
 *   wait for timeout and T3 got the mutex.
 * @ingroup kernel_mutex_tests
 */
ZTEST_USER(mutex_api_1cpu, test_mutex_priority_inheritance)
{
	/**TESTPOINT: run test case 1, given priority T1 < T2 */
	k_mutex_init(&tmutex);

	/* we told thread which case runs now */
	case_type = 1;

	/* spawn a lower priority thread t1 for holding the mutex */
	k_thread_create(&tdata, tstack, STACK_SIZE,
		tThread_T1_priority_inheritance,
			&tmutex, &tdata, NULL,
			K_PRIO_PREEMPT(THREAD_LOW_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	/* wait for spawn thread t1 to take action */
	k_msleep(TIMEOUT);

	/**TESTPOINT: The current thread does not own the mutex.*/
	zassert_true(k_mutex_unlock(&tmutex) == -EPERM,
			"fail: current thread does not own the mutex");

	/* spawn a higher priority thread t2 for holding the mutex */
	k_thread_create(&tdata2, tstack2, STACK_SIZE,
		tThread_T2_priority_inheritance,
			&tmutex, &tdata2, NULL,
			K_PRIO_PREEMPT(THREAD_HIGH_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	/* wait for spawn thread t2 to take action */
	k_msleep(TIMEOUT+1000);

	/**TESTPOINT: run test case 2, given priority T1 > T2, this means
	 * priority inheritance won't happen.
	 */
	k_mutex_init(&tmutex);
	case_type = 2;

	/* spawn a lower priority thread t1 for holding the mutex */
	k_thread_create(&tdata, tstack, STACK_SIZE,
		tThread_T1_priority_inheritance,
			&tmutex, &tdata, NULL,
			K_PRIO_PREEMPT(THREAD_HIGH_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	/* wait for spawn thread t1 to take action */
	k_msleep(TIMEOUT);

	/* spawn a higher priority thread t2 for holding the mutex */
	k_thread_create(&tdata2, tstack2, STACK_SIZE,
		tThread_T2_priority_inheritance,
			&tmutex, &tdata2, NULL,
			K_PRIO_PREEMPT(THREAD_LOW_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	/* wait for spawn thread t2 to take action */
	k_msleep(TIMEOUT+1000);

	/**TESTPOINT: run test case 3, given priority T1 < T3 < T2, but t2 do
	 * not get mutex due to timeout.
	 */
	k_mutex_init(&tmutex);
	case_type = 3;

	/* spawn a lower priority thread t1 for holding the mutex */
	k_thread_create(&tdata, tstack, STACK_SIZE,
		tThread_T1_priority_inheritance,
			&tmutex, &tdata, NULL,
			K_PRIO_PREEMPT(THREAD_LOW_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	/* wait for spawn thread t1 to take action */
	k_msleep(TIMEOUT);

	/* spawn a higher priority thread t2 for holding the mutex */
	k_thread_create(&tdata2, tstack2, STACK_SIZE,
		tThread_T2_priority_inheritance,
			&tmutex, &tdata2, NULL,
			K_PRIO_PREEMPT(THREAD_HIGH_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	/* spawn a higher priority thread t3 for holding the mutex */
	k_thread_create(&tdata3, tstack3, STACK_SIZE,
		tThread_lock_with_time_period,
			&tmutex, &tdata3, NULL,
			K_PRIO_PREEMPT(THREAD_MID_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	/* wait for spawn thread t2 and t3 to take action */
	k_msleep(TIMEOUT+1000);
}

static void tThread_mutex_lock_should_fail(void *p1, void *p2, void *p3)
{
	k_timeout_t timeout;
	struct k_mutex *mutex = (struct k_mutex *)p1;

	timeout.ticks = 0;
	timeout.ticks |= (uint64_t)(uintptr_t)p2 << 32;
	timeout.ticks |= (uint64_t)(uintptr_t)p3 << 0;

	zassert_equal(-EAGAIN, k_mutex_lock(mutex, timeout), NULL);
}

/**
 * @brief Test fix for subtle race during priority inversion
 *
 * - A low priority thread (Tlow) locks mutex A.
 * - A high priority thread (Thigh) blocks on mutex A, boosting the priority
 *   of Tlow.
 * - Thigh times out waiting for mutex A.
 * - Before Thigh has a chance to execute, Tlow unlocks mutex A (which now
 *   has no owner) and drops its own priority.
 * - Thigh now gets a chance to execute and finds that it timed out, and
 *   then enters the block of code to lower the priority of the thread that
 *   owns mutex A (now nobody).
 * - Thigh tries to the dereference the owner of mutex A (which is nobody,
 *   and thus it is NULL). This leads to an exception.
 *
 * @ingroup kernel_mutex_tests
 *
 * @see k_mutex_lock()
 */
ZTEST(mutex_api_1cpu, test_mutex_timeout_race_during_priority_inversion)
{
	k_timeout_t timeout;
	uintptr_t timeout_upper;
	uintptr_t timeout_lower;
	int helper_prio = k_thread_priority_get(k_current_get()) + 1;

	k_mutex_init(&tmutex);

	/* align to tick boundary */
	k_sleep(K_TICKS(1));

	/* allow non-kobject data to be shared (via registers) */
	timeout = K_TIMEOUT_ABS_TICKS(k_uptime_ticks()
		+ CONFIG_TEST_MUTEX_API_THREAD_CREATE_TICKS);
	timeout_upper = timeout.ticks >> 32;
	timeout_lower = timeout.ticks & BIT64_MASK(32);

	k_mutex_lock(&tmutex, K_FOREVER);
	k_thread_create(&tdata, tstack, K_THREAD_STACK_SIZEOF(tstack),
			tThread_mutex_lock_should_fail, &tmutex, (void *)timeout_upper,
			(void *)timeout_lower, helper_prio,
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_priority_set(k_current_get(), K_HIGHEST_THREAD_PRIO);

	k_sleep(timeout);

	k_mutex_unlock(&tmutex);
}

static void *mutex_api_tests_setup(void)
{
#ifdef CONFIG_USERSPACE
	k_thread_access_grant(k_current_get(), &tdata, &tstack, &tdata2,
				&tstack2, &tdata3, &tstack3, &kmutex,
				&tmutex);
#endif
	return NULL;
}

ZTEST_SUITE(mutex_api, NULL, mutex_api_tests_setup, NULL, NULL, NULL);
ZTEST_SUITE(mutex_api_1cpu, NULL, mutex_api_tests_setup,
			ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
