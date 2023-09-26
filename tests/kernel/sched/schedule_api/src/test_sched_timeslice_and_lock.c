/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_sched.h"
#define THREADS_NUM     3
#define DURATION	K_MSEC(1)

BUILD_ASSERT(THREADS_NUM <= MAX_NUM_THREAD);

static struct thread_data tdata[THREADS_NUM];
static struct k_thread tthread[THREADS_NUM];
static int old_prio, init_prio;

struct k_thread t;

K_SEM_DEFINE(pend_sema, 0, 1);
K_SEM_DEFINE(timer_sema, 0, 1);
struct k_timer th_wakeup_timer;

static void thread_entry(void *p1, void *p2, void *p3)
{
	int sleep_ms = POINTER_TO_INT(p2);

	if (sleep_ms > 0) {
		k_msleep(sleep_ms);
	}

	int tnum = POINTER_TO_INT(p1);

	tdata[tnum].executed = 1;
}

static void setup_threads(void)
{
	old_prio = k_thread_priority_get(k_current_get());
	for (int i = 0; i < THREADS_NUM; i++) {
		if (i == 0) {
			/* spawn thread with higher priority */
			tdata[i].priority = init_prio - 1;
		} else if (i == 1) {
			/* spawn thread with same priority */
			tdata[i].priority = init_prio;
		} else {
			/* spawn thread with lower priority */
			tdata[i].priority = init_prio + 1;
		}
		tdata[i].executed = 0;
	}
	k_thread_priority_set(k_current_get(), init_prio);
}

static void spawn_threads(int sleep_sec)
{
	for (int i = 0; i < THREADS_NUM; i++) {
		tdata[i].tid = k_thread_create(&tthread[i], tstacks[i],
					       STACK_SIZE, thread_entry,
					       INT_TO_POINTER(i),
					       INT_TO_POINTER(sleep_sec),
					       NULL, tdata[i].priority, 0,
					       K_NO_WAIT);
	}
}

static void teardown_threads(void)
{
	for (int i = 0; i < THREADS_NUM; i++) {
		k_thread_abort(tdata[i].tid);
	}
	k_thread_priority_set(k_current_get(), old_prio);
}

static void timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	k_sem_give(&timer_sema);
}

static void thread_handler(void *p1, void *p2, void *p3)
{
	k_timer_init(&th_wakeup_timer, timer_handler, NULL);
	k_timer_start(&th_wakeup_timer, DURATION, K_NO_WAIT);
}

/* test cases */

/**
 * @brief Validate the behavior of cooperative thread
 * when it yields
 *
 * @ingroup kernel_sched_tests
 *
 * @details Create 3 threads of priority -2, -1 and 0.
 * Yield the main thread which is cooperative. Check
 * if all the threads gets executed.
 */
ZTEST(threads_scheduling, test_yield_cooperative)
{

	/* set current thread to a cooperative priority */
	init_prio = -1;
	setup_threads();

	spawn_threads(0);
	/* checkpoint: only higher priority thread get executed when yield */
	k_yield();
	zassert_true(tdata[0].executed == 1);
	zassert_true(tdata[1].executed == 1);
	for (int i = 2; i < THREADS_NUM; i++) {
		zassert_true(tdata[i].executed == 0);
	}
	/* restore environment */
	teardown_threads();
}
/**
 * @brief Validate the behavior of cooperative thread when it sleeps
 *
 * @details Create 3 threads of priority -2, -1 and 0. Put the main
 * thread in timeout queue by calling k_sleep() which is cooperative.
 * Check if all the threads gets executed.
 *
 * @ingroup kernel_sched_tests
 */
ZTEST(threads_scheduling, test_sleep_cooperative)
{
	/* set current thread to a cooperative priority */
	init_prio = -1;
	setup_threads();

	spawn_threads(0);
	/* checkpoint: all ready threads get executed when k_sleep */
	k_sleep(K_MSEC(100));
	for (int i = 0; i < THREADS_NUM; i++) {
		zassert_true(tdata[i].executed == 1);
	}

	/* restore environment */
	teardown_threads();
}

ZTEST(threads_scheduling, test_busy_wait_cooperative)
{
	/* set current thread to a cooperative priority */
	init_prio = -1;
	setup_threads();

	spawn_threads(0);
	k_busy_wait(100000); /* 100 ms */
	/* checkpoint: No other threads get executed */
	for (int i = 0; i < THREADS_NUM; i++) {
		zassert_true(tdata[i].executed == 0);
	}
	/* restore environment */
	teardown_threads();
}

/**
 * @brief Validate k_wakeup()
 *
 * @details Create 3 threads with main thread with priority 0
 * and other threads with -1, 0 ,+1 priority. Now -1 priority
 * thread gets executed and it is made to sleep for 10 sec.
 * Now, wake up the -1 priority thread and check if it starts
 * executing.
 *
 * @see k_wakeup()
 *
 * @ingroup kernel_sched_tests
 */
ZTEST(threads_scheduling, test_sleep_wakeup_preemptible)
{
	/* set current thread to a preemptible priority */
	init_prio = 0;
	setup_threads();

	spawn_threads(10 * 1000); /* 10 second */
	/* checkpoint: lower threads not executed, high threads are in sleep */
	for (int i = 0; i < THREADS_NUM; i++) {
		zassert_true(tdata[i].executed == 0);
	}
	k_wakeup(tdata[0].tid);
	zassert_true(tdata[0].executed == 1);
	/* restore environment */
	teardown_threads();
}

static int executed;
static void coop_thread(void *p1, void *p2, void *p3)
{
	k_sem_take(&pend_sema, K_MSEC(100));
	executed = 1;
}

/**
 * @brief Verify k_wakeup() behavior on pending thread
 *
 * @details The test creates a cooperative thread and let
 * it wait for semaphore. Then calls k_wakeup(). The k_wakeup()
 * call should return gracefully without waking up the thread
 *
 * @see k_wakeup()
 *
 * @ingroup kernel_sched_tests
 */
ZTEST(threads_scheduling, test_pending_thread_wakeup)
{
	/* Make current thread preemptible */
	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(1));

	/* Create a thread which waits for semaphore */
	k_tid_t tid = k_thread_create(&t, tstack, STACK_SIZE,
				      (k_thread_entry_t)coop_thread,
				      NULL, NULL, NULL,
				      K_PRIO_COOP(1), 0, K_NO_WAIT);

	zassert_false(executed == 1, "The thread didn't wait"
		      " for semaphore acquisition");

	/* Call wakeup on pending thread */
	k_wakeup(tid);

	/* TESTPOINT: k_wakeup() shouldn't resume
	 * execution of pending thread
	 */
	zassert_true(executed != 1, "k_wakeup woke up a"
		     " pending thread!");

	k_thread_abort(tid);
}

/**
 * @brief Validate preemptive thread behavior with time slice
 *
 * @details Create 3 threads with -1, 0, and 1 as priority, setup
 * time slice for threads with priority 0. Make sure the threads
 * with equal priorities are executed in time slice.
 *
 * @ingroup kernel_sched_tests
 */
ZTEST(threads_scheduling, test_time_slicing_preemptible)
{
#ifdef CONFIG_TIMESLICING
	/* set current thread to a preemptible priority */
	init_prio = 0;
	setup_threads();

	k_sched_time_slice_set(200, 0); /* 200 ms */
	spawn_threads(0);
	/* checkpoint: higher priority threads get executed immediately */
	zassert_true(tdata[0].executed == 1);
	k_busy_wait(500000); /* 500 ms */
	/* checkpoint: equal priority threads get executed every time slice */
	zassert_true(tdata[1].executed == 1);
	for (int i = 2; i < THREADS_NUM; i++) {
		zassert_true(tdata[i].executed == 0);
	}

	/* restore environment */
	k_sched_time_slice_set(0, 0); /* disable time slice */
	teardown_threads();
#else /* CONFIG_TIMESLICING */
	ztest_test_skip();
#endif /* CONFIG_TIMESLICING */
}

/**
 * @brief Check the behavior of preemptive thread with k_busy_wait()
 *
 * @details Create 3 threads with -1, 0, and 1 as priority,
 * setup time slice for threads with priority 0. Make sure the
 * threads with equal priorities are executed in time slice.
 * Also run k_busy_wait() for 5 secs and check if other threads
 * are not executed at that time.
 *
 * @see k_busy_wait()
 *
 * @ingroup kernel_sched_tests
 */
ZTEST(threads_scheduling, test_time_slicing_disable_preemptible)
{
#ifdef CONFIG_TIMESLICING
	/* set current thread to a preemptible priority */
	init_prio = 0;
	setup_threads();

	spawn_threads(0);
	/* checkpoint: higher priority threads get executed immediately */
	zassert_true(tdata[0].executed == 1);
	k_busy_wait(500000); /* 500 ms */
	/* checkpoint: equal priority threads get executed every time slice */
	zassert_true(tdata[1].executed == 0);
	for (int i = 2; i < THREADS_NUM; i++) {
		zassert_true(tdata[i].executed == 0);
	}
	/* restore environment */
	teardown_threads();
#else /* CONFIG_TIMESLICING */
	ztest_test_skip();
#endif /* CONFIG_TIMESLICING */
}

/**
 * @brief Lock the scheduler when preemptive threads are running
 *
 * @details Create 3 threads and lock the scheduler. Make sure that the
 * threads are not executed. Call k_sleep() and check if the threads
 * have executed.
 *
 * @ingroup kernel_sched_tests
 */
ZTEST(threads_scheduling, test_lock_preemptible)
{
	/* set current thread to a preemptible priority */
	init_prio = 0;
	setup_threads();

	k_sched_lock();
	spawn_threads(0);
	/* do critical thing */
	k_busy_wait(100000);
	/* checkpoint: all other threads not been executed */
	for (int i = 0; i < THREADS_NUM; i++) {
		zassert_true(tdata[i].executed == 0);
	}
	/* make current thread unready */
	k_sleep(K_MSEC(100));
	/* checkpoint: all other threads get executed */
	for (int i = 0; i < THREADS_NUM; i++) {
		zassert_true(tdata[i].executed == 1);
	}
	/* restore environment */
	teardown_threads();
}

/**
 * @brief Validate k_sched_lock() and k_sched_unlock()
 *
 * @details  Lock the scheduler and create 3 threads. Check
 * that the threads are not executed. Call k_sched_unlock()
 * and check if the threads have executed.
 *
 * @see k_sched_lock(), k_sched_unlock()
 *
 * @ingroup kernel_sched_tests
 */
ZTEST(threads_scheduling, test_unlock_preemptible)
{
	/* set current thread to a preemptible priority */
	init_prio = 0;
	setup_threads();

	k_sched_lock();
	spawn_threads(0);
	/* do critical thing */
	k_busy_wait(100000);

	k_sched_unlock();

	/* ensure threads of equal priority can run */
	k_yield();

	/* checkpoint: higher and equal threads get executed */
	zassert_true(tdata[0].executed == 1);
	zassert_true(tdata[1].executed == 1);
	zassert_true(tdata[2].executed == 0);

	/* restore environment */
	teardown_threads();
}

/**
 * @brief Validate nested k_sched_lock() and k_sched_unlock()
 *
 * @details In a preemptive thread, lock the scheduler twice and
 * create a cooperative thread.  Call k_sched_unlock() and check the
 * cooperative thread haven't executed.  Unlock it again to see the
 * thread have executed this time.
 *
 * @see k_sched_lock(), k_sched_unlock()
 *
 * @ingroup kernel_sched_tests
 */
ZTEST(threads_scheduling, test_unlock_nested_sched_lock)
{
	/* set current thread to a preemptible priority */
	init_prio = 0;
	setup_threads();

	/* take the scheduler lock twice */
	k_sched_lock();
	k_sched_lock();

	/* spawn threads without wait */
	spawn_threads(0);

	/* do critical thing */
	k_busy_wait(100000);

	/* unlock once; this shouldn't let other threads to run */
	k_sched_unlock();

	/* checkpoint: no threads get executed */
	for (int i = 0; i < THREADS_NUM; i++) {
		zassert_true(tdata[i].executed == 0);
	}

	/* unlock another; this let the higher thread to run */
	k_sched_unlock();

	/* Ensure threads of equal priority run */
	k_yield();

	/* checkpoint: higher threads NOT get executed */
	zassert_true(tdata[0].executed == 1);
	zassert_true(tdata[1].executed == 1);
	zassert_true(tdata[2].executed == 0);

	/* restore environment */
	teardown_threads();
}

/**
 * @brief validate k_wakeup() in some corner scenario
 * @details trigger a timer and after expiration of timer
 * call k_wakeup(), even the thread is not in sleep state neither
 * in pending state
 *
 * @see k_wakeup()
 *
 * @ingroup kernel_sched_tests
 */
ZTEST(threads_scheduling, test_wakeup_expired_timer_thread)
{
	k_tid_t tid = k_thread_create(&tthread[0], tstack, STACK_SIZE,
					thread_handler, NULL, NULL, NULL,
					K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	k_sem_take(&timer_sema, K_FOREVER);
	/* wakeup a thread if the timer is expired */
	k_wakeup(tid);
	k_thread_abort(tid);
}
