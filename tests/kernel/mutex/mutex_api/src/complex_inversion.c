/*
 * Copyright 2024 by Garmin Ltd. or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test complex mutex priority inversion
 *
 * This module demonstrates the kernel's priority inheritance algorithm
 * with two mutexes and four threads, ensuring that boosting priority of
 * a thread waiting on another mutex does not break assumptions of the
 * mutex's waitq, causing the incorrect thread to run or a crash.
 *
 * Sequence for priority inheritance testing:
 *  - thread_08 takes mutex_1
 *  - thread_07 takes mutex_0 then waits on mutex_1
 *  - thread_06 waits on mutex_1
 *  - thread_05 waits on mutex_0, boosting priority of thread_07
 *  - thread_08 gives mutex_1, thread_07 takes mutex_1
 *  - thread_07 gives mutex_1, thread_06 takes mutex_1
 *  - thread_07 gives mutex_0, thread_05 takes mutex_0
 *  - thread_06 gives mutex_1
 *  - thread_05 gives mutex_0
 */

#include <zephyr/tc_util.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/mutex.h>

#define STACKSIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)

static ZTEST_DMEM int tc_rc = TC_PASS; /* test case return code */

static K_MUTEX_DEFINE(mutex_0);
static K_MUTEX_DEFINE(mutex_1);

#define PARTICIPANT_THREAD_OPTIONS (K_INHERIT_PERMS)

#define DEFINE_PARTICIPANT_THREAD(id)                                                              \
	static K_THREAD_STACK_DEFINE(thread_##id##_stack_area, STACKSIZE);                         \
	static struct k_thread thread_##id##_thread_data;                                          \
	static k_tid_t thread_##id##_tid;                                                          \
	static K_SEM_DEFINE(thread_##id##_wait, 0, 1);                                             \
	static K_SEM_DEFINE(thread_##id##_done, 0, 1);

#define CREATE_PARTICIPANT_THREAD(id, pri)                                                         \
	thread_##id##_tid = k_thread_create(&thread_##id##_thread_data, thread_##id##_stack_area,  \
					    K_THREAD_STACK_SIZEOF(thread_##id##_stack_area),       \
					    (k_thread_entry_t)thread_##id, &thread_##id##_wait,    \
					    &thread_##id##_done, NULL, pri,                        \
					    PARTICIPANT_THREAD_OPTIONS, K_FOREVER);                \
	k_thread_name_set(thread_##id##_tid, "thread_" STRINGIFY(id));
#define START_PARTICIPANT_THREAD(id) k_thread_start(&(thread_##id##_thread_data));
#define JOIN_PARTICIPANT_THREAD(id)  k_thread_join(&(thread_##id##_thread_data), K_FOREVER);

#define WAIT_FOR_MAIN()                                                                            \
	k_sem_give(done);                                                                          \
	k_sem_take(wait, K_FOREVER);

#define ADVANCE_THREAD(id)                                                                         \
	SIGNAL_THREAD(id);                                                                         \
	WAIT_FOR_THREAD(id);

#define SIGNAL_THREAD(id) k_sem_give(&thread_##id##_wait);

#define WAIT_FOR_THREAD(id) zassert_ok(k_sem_take(&thread_##id##_done, K_MSEC(100)));

/**
 *
 * thread_05 -
 *
 */

static void thread_05(struct k_sem *wait, struct k_sem *done)
{
	int rv;

	/*
	 * Wait for mutex_0, boosting the priority of thread_07 so it will lock mutex_1 first.
	 */

	WAIT_FOR_MAIN();

	rv = k_mutex_lock(&mutex_0, K_FOREVER);
	if (rv != 0) {
		tc_rc = TC_FAIL;
		TC_ERROR("Failed to take mutex %p\n", &mutex_0);
		return;
	}

	WAIT_FOR_MAIN();

	k_mutex_unlock(&mutex_0);
}

/**
 *
 * thread_06 -
 *
 */

static void thread_06(struct k_sem *wait, struct k_sem *done)
{
	int rv;

	/*
	 * Wait for mutex_1. Initially it will be the highest priority waiter, but
	 * thread_07 will be boosted above thread_06 so thread_07 will lock it first.
	 */

	WAIT_FOR_MAIN();

	rv = k_mutex_lock(&mutex_1, K_FOREVER);
	if (rv != 0) {
		tc_rc = TC_FAIL;
		TC_ERROR("Failed to take mutex %p\n", &mutex_1);
		return;
	}

	WAIT_FOR_MAIN();

	k_mutex_unlock(&mutex_1);
}

/**
 *
 * thread_07 -
 *
 */

static void thread_07(struct k_sem *wait, struct k_sem *done)
{
	int rv;

	/*
	 * Lock mutex_0 and wait for mutex_1. After thread_06 is also waiting for
	 * mutex_1, thread_05 will wait for mutex_0, boosting the priority for
	 * thread_07 so it should lock mutex_1 first when it is unlocked by thread_08.
	 */

	WAIT_FOR_MAIN();

	rv = k_mutex_lock(&mutex_0, K_NO_WAIT);
	if (rv != 0) {
		tc_rc = TC_FAIL;
		TC_ERROR("Failed to take mutex %p\n", &mutex_0);
		return;
	}

	WAIT_FOR_MAIN();

	rv = k_mutex_lock(&mutex_1, K_FOREVER);
	if (rv != 0) {
		tc_rc = TC_FAIL;
		TC_ERROR("Failed to take mutex %p\n", &mutex_1);
		k_mutex_unlock(&mutex_0);
		return;
	}

	WAIT_FOR_MAIN();

	k_mutex_unlock(&mutex_1);
	k_mutex_unlock(&mutex_0);
}

/**
 *
 * thread_08 -
 *
 */

static void thread_08(struct k_sem *wait, struct k_sem *done)
{
	int rv;

	/*
	 * Lock mutex_1 and hold until priority has been boosted on thread_07
	 * to ensure that thread_07 is the first to lock mutex_1 when thread_08
	 * unlocks it.
	 */

	WAIT_FOR_MAIN();

	rv = k_mutex_lock(&mutex_1, K_NO_WAIT);
	if (rv != 0) {
		tc_rc = TC_FAIL;
		TC_ERROR("Failed to take mutex %p\n", &mutex_1);
		return;
	}

	WAIT_FOR_MAIN();

	k_mutex_unlock(&mutex_1);
}

DEFINE_PARTICIPANT_THREAD(05);
DEFINE_PARTICIPANT_THREAD(06);
DEFINE_PARTICIPANT_THREAD(07);
DEFINE_PARTICIPANT_THREAD(08);

static void create_participant_threads(void)
{
	CREATE_PARTICIPANT_THREAD(05, 5);
	CREATE_PARTICIPANT_THREAD(06, 6);
	CREATE_PARTICIPANT_THREAD(07, 7);
	CREATE_PARTICIPANT_THREAD(08, 8);
}

static void start_participant_threads(void)
{
	START_PARTICIPANT_THREAD(05);
	START_PARTICIPANT_THREAD(06);
	START_PARTICIPANT_THREAD(07);
	START_PARTICIPANT_THREAD(08);
}

static void join_participant_threads(void)
{
	JOIN_PARTICIPANT_THREAD(05);
	JOIN_PARTICIPANT_THREAD(06);
	JOIN_PARTICIPANT_THREAD(07);
	JOIN_PARTICIPANT_THREAD(08);
}

/**
 *
 * @brief Main thread to test mutex locking
 *
 * This thread orchestrates mutex locking on other threads and verifies that
 * the correct thread is holding mutexes at any given step.
 *
 */

ZTEST(mutex_api, test_complex_inversion)
{
	create_participant_threads();
	start_participant_threads();

	/* Wait for all the threads to start up */
	WAIT_FOR_THREAD(08);
	WAIT_FOR_THREAD(07);
	WAIT_FOR_THREAD(06);
	WAIT_FOR_THREAD(05);

	ADVANCE_THREAD(08); /* thread_08 takes mutex_1 */
	zassert_equal(thread_08_tid, mutex_1.owner, "expected owner %s, not %s\n",
		      thread_08_tid->name, mutex_1.owner->name);

	ADVANCE_THREAD(07); /* thread_07 takes mutex_0 */
	zassert_equal(thread_07_tid, mutex_0.owner, "expected owner %s, not %s\n",
		      thread_07_tid->name, mutex_0.owner->name);

	SIGNAL_THREAD(07);    /* thread_07 waits on mutex_1 */
	k_sleep(K_MSEC(100)); /* Give thread_07 some time to wait on mutex_1 */

	SIGNAL_THREAD(06);    /* thread_06 waits on mutex_1 */
	k_sleep(K_MSEC(100)); /* Give thread_06 some time to wait on mutex_1 */

	SIGNAL_THREAD(05); /* thread_05 waits on mutex_0, boosting priority of thread_07 */

	SIGNAL_THREAD(08); /* thread_08 gives mutex_1 */

	/* If thread_06 erroneously took mutex_1, giving it could cause a crash
	 * when CONFIG_WAITQ_SCALABLE is set. Give it a chance to run to make sure
	 * this crash isn't hit.
	 */
	SIGNAL_THREAD(06);

	WAIT_FOR_THREAD(07); /* thread_07 takes mutex_1 */
	zassert_equal(thread_07_tid, mutex_1.owner, "expected owner %s, not %s\n",
		      thread_07_tid->name, mutex_1.owner->name);

	SIGNAL_THREAD(07);   /* thread_07 gives mutex_1 then gives mutex_0 */
	WAIT_FOR_THREAD(06); /* thread_06 takes mutex_1 */
	WAIT_FOR_THREAD(05); /* thread_05 takes mutex_0 */
	zassert_equal(thread_06_tid, mutex_1.owner, "expected owner %s, not %s\n",
		      thread_06_tid->name, mutex_1.owner->name);
	zassert_equal(thread_05_tid, mutex_0.owner, "expected owner %s, not %s\n",
		      thread_05_tid->name, mutex_0.owner->name);

	SIGNAL_THREAD(06); /* thread_06 gives mutex_1 */
	SIGNAL_THREAD(05); /* thread_05 gives mutex_0 */

	zassert_equal(tc_rc, TC_PASS);

	join_participant_threads();
}
