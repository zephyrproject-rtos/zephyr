/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_sched.h"
#define THREADS_NUM     3
#define DURATION	1
static K_THREAD_STACK_ARRAY_DEFINE(tstack, THREADS_NUM, STACK_SIZE);

static struct thread_data tdata[THREADS_NUM];
static struct k_thread tthread[THREADS_NUM];
static int old_prio, init_prio;

K_THREAD_STACK_DEFINE(t_stack, STACK_SIZE);
struct k_thread t;

K_SEM_DEFINE(pend_sema, 0, 1);
K_SEM_DEFINE(timer_sema, 0, 1);
struct k_timer timer;

static void thread_entry(void *p1, void *p2, void *p3)
{
	int sleep_ms = (int)p2;

	if (sleep_ms > 0) {
		k_sleep(sleep_ms);
	}

	int tnum = (int)p1;

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
		tdata[i].tid = k_thread_create(&tthread[i], tstack[i],
					       STACK_SIZE, thread_entry,
					       (void *)i, (void *)sleep_sec,
					       NULL, tdata[i].priority, 0, 0);
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
	k_timer_init(&timer, timer_handler, NULL);
	k_timer_start(&timer, DURATION, 0);
}

/*test cases*/

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
void test_yield_cooperative(void)
{

	/* set current thread to a cooperative priority */
	init_prio = -1;
	setup_threads();

	spawn_threads(0);
	/* checkpoint: only higher priority thread get executed when yield */
	k_yield();
	zassert_true(tdata[0].executed == 1, NULL);
	zassert_true(tdata[1].executed == 1, NULL);
	for (int i = 2; i < THREADS_NUM; i++) {
		zassert_true(tdata[i].executed == 0, NULL);
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
void test_sleep_cooperative(void)
{
	/* set current thread to a cooperative priority */
	init_prio = -1;
	setup_threads();

	spawn_threads(0);
	/* checkpoint: all ready threads get executed when k_sleep */
	k_sleep(100);
	for (int i = 0; i < THREADS_NUM; i++) {
		zassert_true(tdata[i].executed == 1, NULL);
	}

	/* restore environment */
	teardown_threads();
}

void test_busy_wait_cooperative(void)
{
	/* set current thread to a preemptible priority */
	init_prio = -1;
	setup_threads();

	spawn_threads(0);
	k_busy_wait(100000); /* 100 ms */
	/* checkpoint: No other threads get executed */
	for (int i = 0; i < THREADS_NUM; i++) {
		zassert_true(tdata[i].executed == 0, NULL);
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
void test_sleep_wakeup_preemptible(void)
{
	/* set current thread to a preemptible priority */
	init_prio = 0;
	setup_threads();

	spawn_threads(10 * 1000); /* 10 second */
	/* checkpoint: lower threads not executed, high threads are in sleep */
	for (int i = 0; i < THREADS_NUM; i++) {
		zassert_true(tdata[i].executed == 0, NULL);
	}
	k_wakeup(tdata[0].tid);
	zassert_true(tdata[0].executed == 1, NULL);
	/* restore environment */
	teardown_threads();
}

static int executed;
static void coop_thread(void *p1, void *p2, void *p3)
{
	k_sem_take(&pend_sema, 100);
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
void test_pending_thread_wakeup(void)
{
	/* Make current thread preemptible */
	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(1));

	/* Create a thread which waits for semaphore */
	k_tid_t tid = k_thread_create(&t, t_stack, STACK_SIZE,
				      (k_thread_entry_t)coop_thread,
				      NULL, NULL, NULL,
				      K_PRIO_COOP(1), 0, 0);

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
void test_time_slicing_preemptible(void)
{
	/* set current thread to a preemptible priority */
	init_prio = 0;
	setup_threads();

	k_sched_time_slice_set(200, 0); /* 200 ms */
	spawn_threads(0);
	/* checkpoint: higher priority threads get executed immediately */
	zassert_true(tdata[0].executed == 1, NULL);
	k_busy_wait(500000); /* 500 ms */
	/* checkpoint: equal priority threads get executed every time slice */
	zassert_true(tdata[1].executed == 1, NULL);
	for (int i = 2; i < THREADS_NUM; i++) {
		zassert_true(tdata[i].executed == 0, NULL);
	}

	/* restore environment */
	k_sched_time_slice_set(0, 0); /* disable time slice */
	teardown_threads();
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
void test_time_slicing_disable_preemptible(void)
{
	/* set current thread to a preemptible priority */
	init_prio = 0;
	setup_threads();

	spawn_threads(0);
	/* checkpoint: higher priority threads get executed immediately */
	zassert_true(tdata[0].executed == 1, NULL);
	k_busy_wait(500000); /* 500 ms */
	/* checkpoint: equal priority threads get executed every time slice */
	zassert_true(tdata[1].executed == 0, NULL);
	for (int i = 2; i < THREADS_NUM; i++) {
		zassert_true(tdata[i].executed == 0, NULL);
	}
	/* restore environment */
	teardown_threads();
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
void test_lock_preemptible(void)
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
		zassert_true(tdata[i].executed == 0, NULL);
	}
	/* make current thread unready */
	k_sleep(100);
	/* checkpoint: all other threads get executed */
	for (int i = 0; i < THREADS_NUM; i++) {
		zassert_true(tdata[i].executed == 1, NULL);
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
void test_unlock_preemptible(void)
{
	/* set current thread to a preemptible priority */
	init_prio = 0;
	setup_threads();

	k_sched_lock();
	spawn_threads(0);
	/* do critical thing */
	k_busy_wait(100000);

	k_sched_unlock();
	/* checkpoint: higher threads get executed */
	zassert_true(tdata[0].executed == 1, NULL);
	for (int i = 1; i < THREADS_NUM; i++) {
		zassert_true(tdata[i].executed == 0, NULL);
	}
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
void test_wakeup_expired_timer_thread(void)
{
	k_tid_t tid = k_thread_create(&tthread[0], t_stack, STACK_SIZE,
					thread_handler, NULL, NULL, NULL,
					K_PRIO_PREEMPT(0), 0, 0);
	k_sem_take(&timer_sema, K_FOREVER);
	/* wakeup a thread if the timer is expired */
	k_wakeup(tid);
	k_thread_abort(tid);
}
