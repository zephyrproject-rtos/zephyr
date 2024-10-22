/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/random/random.h>

#define NUM_THREADS 8
/* this should be large enough for us
 * to print a failing assert if necessary
 */
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)

struct k_thread worker_threads[NUM_THREADS];
k_tid_t worker_tids[NUM_THREADS];

K_THREAD_STACK_ARRAY_DEFINE(worker_stacks, NUM_THREADS, STACK_SIZE);

int thread_deadlines[NUM_THREADS];

/* The number of worker threads that ran, and array of their
 * indices in execution order
 */
int n_exec;
int exec_order[NUM_THREADS];

void worker(void *p1, void *p2, void *p3)
{
	int tidx = POINTER_TO_INT(p1);

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	zassert_true(tidx >= 0 && tidx < NUM_THREADS, "");
	zassert_true(n_exec >= 0 && n_exec < NUM_THREADS, "");

	exec_order[n_exec++] = tidx;

	/* Sleep, don't exit.  It's not implausible that some
	 * platforms implement a thread-based cleanup step for threads
	 * that exit (pthreads does this already) which might muck
	 * with the scheduling.
	 */
	while (1) {
		k_sleep(K_MSEC(1000000));
	}
}

ZTEST(suite_deadline, test_deadline)
{
	int i;

	n_exec = 0;

	/* Create a bunch of threads at a single lower priority.  Give
	 * them each a random deadline.  Sleep, and check that they
	 * were executed in the right order.
	 */
	for (i = 0; i < NUM_THREADS; i++) {
		worker_tids[i] = k_thread_create(&worker_threads[i],
				worker_stacks[i], STACK_SIZE,
				worker, INT_TO_POINTER(i), NULL, NULL,
				K_LOWEST_APPLICATION_THREAD_PRIO,
				0, K_NO_WAIT);

		/* Positive-definite number with the bottom 8 bits
		 * masked off to prevent aliasing where "very close"
		 * deadlines end up in the opposite order due to the
		 * changing "now" between calls to
		 * k_thread_deadline_set().
		 *
		 * Use only 30 bits of significant value.  The API
		 * permits 31 (strictly: the deadline time of the
		 * "first" runnable thread in any given priority and
		 * the "last" must be less than 2^31), but because the
		 * time between our generation here and the set of the
		 * deadline below takes non-zero time, it's possible
		 * to see rollovers.  Easier than using a modulus test
		 * or whatnot to restrict the values.
		 */
		thread_deadlines[i] = sys_rand32_get() & 0x3fffff00;
	}

	zassert_true(n_exec == 0, "threads ran too soon");

	/* Similarly do the deadline setting in one quick pass to
	 * minimize aliasing with "now"
	 */
	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_deadline_set(&worker_threads[i], thread_deadlines[i]);
	}

	zassert_true(n_exec == 0, "threads ran too soon");

	k_sleep(K_MSEC(100));

	zassert_true(n_exec == NUM_THREADS, "not enough threads ran");

	for (i = 1; i < NUM_THREADS; i++) {
		int d0 = thread_deadlines[exec_order[i-1]];
		int d1 = thread_deadlines[exec_order[i]];

		zassert_true(d0 <= d1, "threads ran in wrong order");
	}
	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_abort(worker_tids[i]);
	}
}

void yield_worker(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	zassert_true(n_exec >= 0 && n_exec < NUM_THREADS, "");

	n_exec += 1;

	k_yield();

	/* should not get here until all threads have started */
	zassert_true(n_exec == NUM_THREADS, "");

	k_thread_abort(k_current_get());

	CODE_UNREACHABLE;
}

ZTEST(suite_deadline, test_yield)
{
	/* Test that yield works across threads with the
	 * same deadline and priority. This currently works by
	 * simply not setting a deadline, which results in a
	 * deadline of 0.
	 */

	int i;

	n_exec = 0;

	/* Create a bunch of threads at a single lower priority
	 * and deadline.
	 * Each thread increments its own variable, then yields
	 * to the next. Sleep. Check that all threads ran.
	 */
	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_create(&worker_threads[i],
				worker_stacks[i], STACK_SIZE,
				yield_worker, NULL, NULL, NULL,
				K_LOWEST_APPLICATION_THREAD_PRIO,
				0, K_NO_WAIT);
	}

	zassert_true(n_exec == 0, "threads ran too soon");

	k_sleep(K_MSEC(100));

	zassert_true(n_exec == NUM_THREADS, "not enough threads ran");
}

void unqueue_worker(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	zassert_true(n_exec >= 0 && n_exec < NUM_THREADS, "");

	n_exec += 1;
}

/**
 * @brief Validate the behavior of deadline_set when the thread is not queued
 *
 * @details Create a bunch of threads with scheduling delay which make the
 * thread in unqueued state. The k_thread_deadline_set() call should not make
 * these threads run before there delay time pass.
 *
 * @ingroup kernel_sched_tests
 */
ZTEST(suite_deadline, test_unqueued)
{
	int i;

	n_exec = 0;

	for (i = 0; i < NUM_THREADS; i++) {
		worker_tids[i] = k_thread_create(&worker_threads[i],
				worker_stacks[i], STACK_SIZE,
				unqueue_worker, NULL, NULL, NULL,
				K_LOWEST_APPLICATION_THREAD_PRIO,
				0, K_MSEC(100));
	}

	zassert_true(n_exec == 0, "threads ran too soon");

	for (i = 0; i < NUM_THREADS; i++) {
		thread_deadlines[i] = sys_rand32_get() & 0x3fffff00;
		k_thread_deadline_set(&worker_threads[i], thread_deadlines[i]);
	}

	// k_sleep(K_MSEC(50));

	// zassert_true(n_exec == 0, "deadline set make the unqueued thread run");

	k_sleep(K_MSEC(100));
	printk("n_exec:%d\n", n_exec);
	zassert_true(n_exec == NUM_THREADS, "not enough threads ran");

	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_abort(worker_tids[i]);
	}
}

#include <zephyr/kernel.h>
/*
	Remember that for "pure" EDF results, all
	user threads must have the same static priority.
	That happens because EDF will be a tie-breaker
	among two or more ready tasks of the same static
	priority. An arbitrary positive number is chosen here.
*/

#define EDF_PRIORITY 5
#define INACTIVE -1
#define MSEC_TO_CYC(msec)       ((msec * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) / MSEC_PER_SEC)
#define MSEC_TO_USEC(msec)      (msec * 1000)

typedef struct {
    int id;
    int32_t rel_deadline_msec;
    int32_t period_msec;
    uint32_t wcet_msec;
    k_tid_t thread;
    struct k_msgq queue;
} edf_t;

// edf-related structs with the properties for each thread ////////////////////////////////

edf_t task1 = {
	.id = 1,
	.rel_deadline_msec = 10,
	.wcet_msec = 1
};

edf_t task2 = {
	.id = 2,
	.rel_deadline_msec = 50,
	.wcet_msec = 5
};

// timer-related functions ///////////////////////////////////////////////////////////////////

void grow(edf_t *task){
	task->wcet_msec += 1;
	task->rel_deadline_msec += 10;
}


void trigger(edf_t *task){
	const char t = 1;
	
	int deadline = (int) MSEC_TO_CYC(task->rel_deadline_msec);
	
	k_msgq_put(&task->queue, &t, K_NO_WAIT);
	k_thread_deadline_set(task->thread, deadline);
	
	// Uncomment following line for enable rescheduling workaround
    // { struct k_sem dummy_sem; k_sem_init(&dummy_sem, 1, 1); k_sem_give(&dummy_sem); }
	printk("trigger is in ISR:%d\n", k_is_in_isr());
}


void trigger_all(struct k_timer *timer){
	printk("\n");
	trigger(&task1);
	trigger(&task2);
	grow(&task1);   //task 1 progressively gains +10ms of relative deadline
}

// thread function /////////////////////////////////////////////////////////////////////////

void thread_function(void *task_props, void *a2, void *a3){
	char buffer[10];
	char message;
	uint32_t deadline = 0;
	uint64_t start = 0;
	uint64_t end = 0;

	edf_t *task = (edf_t *) task_props;
	task->thread = k_current_get();
	k_msgq_init(&task->queue, buffer, sizeof(char), 10);
	printk("[ %d ] ready.\n", task->id);

	for(;;){
		/*
			The periodic thread has no period, ironically.
			What makes it periodic is the master timer
			associated with it that regularly sends new
			messages to the thread queue (e.g. every two
			seconds).
		*/
		k_msgq_get(&task->queue, &message, K_FOREVER);
		start = k_cycle_get_64();
		deadline = task->thread->base.prio_deadline;
		k_busy_wait(MSEC_TO_USEC(task->wcet_msec));
		end = k_cycle_get_64();
		printk("[ %d ]\tstart %llu\t end %llu\t dead %u k_is_in_isr:%d\n", task->id, start, end, deadline, k_is_in_isr());
	}
}

// defines and main //////////////////////////////////////////////////////////////////////

K_THREAD_DEFINE(
	task1_thread, 2048,
	thread_function,
	&task1, NULL, NULL,
	EDF_PRIORITY, 0, INACTIVE
);


K_THREAD_DEFINE(
	task2_thread, 2048,
	thread_function,
	&task2, NULL, NULL,
	EDF_PRIORITY, 0, INACTIVE
);


K_TIMER_DEFINE(master_timer, trigger_all, NULL);

ZTEST(suite_deadline, test_msg_deadline){
         k_sleep(K_SECONDS(1));
        printk("\nexecuting on: %s\n\n", CONFIG_BOARD_TARGET);

	k_thread_start(task1_thread);
	k_thread_start(task2_thread);
	k_sleep(K_SECONDS(1));
	
	k_timer_start(&master_timer, K_SECONDS(1), K_SECONDS(2));
}

ZTEST_SUITE(suite_deadline, NULL, NULL, NULL, NULL, NULL);
