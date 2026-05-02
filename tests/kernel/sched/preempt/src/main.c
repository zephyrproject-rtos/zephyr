/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/irq_offload.h>
#include <zephyr/kernel_structs.h> /* for _THREAD_PENDING */

/* Explicit preemption test.  Works by creating a set of threads in
 * each priority class (cooperative, preemptive, metairq) which all go
 * to sleep.  Then one is woken up (from a low priority manager
 * thread) and arranges to wake up one other thread and validate that
 * the next thread to be run is correct according to the documented
 * rules.
 *
 * The wakeup test is repeated for all four combinations of threads
 * either holding or not holding the scheduler lock, and by a
 * synchronous wake vs. a wake in a (offloaded) interrupt.
 */

#if defined(CONFIG_SMP) && CONFIG_MP_MAX_NUM_CPUS > 1
#error Preemption test requires single-CPU operation
#endif

#if CONFIG_NUM_METAIRQ_PRIORITIES < 1
#error Need one metairq priority
#endif

#if CONFIG_NUM_COOP_PRIORITIES < 2
#error Need two cooperative priorities
#endif

#if CONFIG_NUM_PREEMPT_PRIORITIES < 2
#error Need two preemptible priorities
#endif

/* Two threads at each priority (to test the case of waking up a
 * thread of equal priority).  But only one metairq, as it isn't
 * technically legal to have more than one at the same priority.
 */
const enum { METAIRQ, COOP, PREEMPTIBLE } worker_priorities[] = {
	METAIRQ,
	COOP, COOP,
	PREEMPTIBLE, PREEMPTIBLE,
};

#define NUM_THREADS ARRAY_SIZE(worker_priorities)

#define STACK_SIZE (640 + CONFIG_TEST_EXTRA_STACK_SIZE)

k_tid_t last_wakeup_thread;

struct k_thread manager_thread;

K_THREAD_STACK_DEFINE(manager_stack, STACK_SIZE);

struct k_thread worker_threads[NUM_THREADS];

K_THREAD_STACK_ARRAY_DEFINE(worker_stacks, NUM_THREADS, STACK_SIZE);

struct k_thread manager_thread;

struct k_sem worker_sems[NUM_THREADS];

/* Command to worker: who to wake up */
int wakeup_target;

/* Command to worker: use a sched_lock()? */
volatile int do_lock;

/* Command to worker: use irq_offload() to indirect the wakeup? */
volatile int do_irq;

/* Command to worker: sleep after wakeup? */
volatile int do_sleep;

/* Command to worker: yield after wakeup? */
volatile int do_yield;

K_SEM_DEFINE(main_sem, 0, 1);

void wakeup_src_thread(int id)
{
	volatile k_tid_t src_thread = &worker_threads[id];

	zassert_true(k_current_get() == &manager_thread, "");

	/* irq_offload() on ARM appears not to do what we want.  It
	 * doesn't appear to go through the normal exception return
	 * path and always returns back into the calling context, so
	 * it can't be used to fake preemption.
	 */
	if (do_irq && IS_ENABLED(CONFIG_ARM)) {
		return;
	}

	last_wakeup_thread = NULL;

	/* A little bit of white-box inspection: check that all the
	 * worker threads are pending.
	 */
	for (int i = 0; i < NUM_THREADS; i++) {
		k_tid_t th = &worker_threads[i];
		char buffer[16];
		const char *str;

		str = k_thread_state_str(th, buffer, sizeof(buffer));
		zassert_not_null(strstr(str, "pending"),
				 "worker thread %d not pending?", i);
	}

	/* Wake the src worker up */
	last_wakeup_thread = NULL;
	k_sem_give(&worker_sems[id]);

	while (do_sleep && !(src_thread->base.thread_state & _THREAD_PENDING)) {
		/* spin, waiting on the sleep timeout */
		Z_SPIN_DELAY(50);
	}

	/* We are lowest priority, SOMEONE must have run */
	zassert_true(!!last_wakeup_thread, "");
}

void manager(void *p1, void *p2, void *p3)
{
	for (int src = 0; src < NUM_THREADS; src++) {
		for (wakeup_target = 0; wakeup_target < NUM_THREADS; wakeup_target++) {

			if (src == wakeup_target) {
				continue;
			}

			for (do_lock = 0; do_lock < 2; do_lock++) {
				for (do_irq = 0; do_irq < 2; do_irq++) {
					do_yield = 0;
					do_sleep = 0;
					wakeup_src_thread(src);

					do_yield = 1;
					do_sleep = 0;
					wakeup_src_thread(src);

					do_yield = 0;
					do_sleep = 1;
					wakeup_src_thread(src);
				}
			}
		}
	}

	k_sem_give(&main_sem);
}

void irq_waker(const void *p)
{
	ARG_UNUSED(p);
	k_sem_give(&worker_sems[wakeup_target]);
}

#define PRI(n) (worker_priorities[n])

void validate_wakeup(int src, int target, k_tid_t last_thread)
{
	int preempted = &worker_threads[target] == last_thread;
	int src_wins = PRI(src) < PRI(target);
	int target_wins = PRI(target) < PRI(src);
	int tie = PRI(src) == PRI(target);

	if (do_sleep) {
		zassert_true(preempted, "sleeping must let any worker run");
		return;
	}

	if (do_yield) {
		if (preempted) {
			zassert_false(src_wins,
				      "src (pri %d) should not have yielded to tgt (%d)",
				      PRI(src), PRI(target));
		} else {
			zassert_true(src_wins,
				      "src (pri %d) should have yielded to tgt (%d)",
				      PRI(src), PRI(target));
		}

		return;
	}

	if (preempted) {
		zassert_true(target_wins, "preemption must raise priority");
	}

	if (PRI(target) == METAIRQ) {
		zassert_true(preempted,
			     "metairq threads must always preempt");
	} else {
		zassert_false(do_lock && preempted,
			      "threads holding scheduler lock must not be preempted");

		zassert_false(preempted && src_wins,
			      "lower priority threads must never preempt");

		if (!do_lock) {
			zassert_false(!preempted && target_wins,
				      "higher priority thread should have preempted");

			/* The scheduler implements a 'first added to
			 * queue' policy for threads within a single
			 * priority, so the last thread woken up (the
			 * target) must never run before the source
			 * thread.
			 *
			 * NOTE: I checked, and Zephyr doesn't
			 * actually document this behavior, though a
			 * few other tests rely on it IIRC.  IMHO
			 * there are good arguments for either this
			 * policy OR the opposite ("run newly woken
			 * threads first"), and long term we may want
			 * to revisit this particular check and maybe
			 * make the policy configurable.
			 */
			zassert_false(preempted && tie,
				      "tied priority should not preempt");
		}
	}
}

void worker(void *p1, void *p2, void *p3)
{
	int id = POINTER_TO_INT(p1);
	k_tid_t curr = &worker_threads[id], prev;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	zassert_true(id >= 0 && id < NUM_THREADS, "");
	zassert_true(curr == k_current_get(), "");

	while (1) {
		/* Wait for the manager or another test thread to wake
		 * us up
		 */
		k_sem_take(&worker_sems[id], K_FOREVER);

		last_wakeup_thread = curr;

		/* If we're the wakeup target, setting last_wakeup_thread is
		 * all we do
		 */
		if (id == wakeup_target) {
			continue;
		}

		if (do_lock) {
			k_sched_lock();
		}

		if (do_irq) {
			/* Do the sem_give() in a IRQ to validate that
			 * ISR return does the right thing
			 */
			irq_offload(irq_waker, NULL);
			prev = last_wakeup_thread;
		} else {
			/* Do the sem_give() directly to validate that
			 * the synchronous scheduling does the right
			 * thing
			 */
			k_sem_give(&worker_sems[wakeup_target]);
			prev = last_wakeup_thread;
		}

		if (do_lock) {
			k_sched_unlock();
		}

		if (do_yield) {
			k_yield();
			prev = last_wakeup_thread;
		}

		if (do_sleep) {
			uint64_t start = k_uptime_get();

			k_sleep(K_MSEC(1));

			zassert_true(k_uptime_get() - start > 0,
				     "didn't sleep");
			prev = last_wakeup_thread;
		}

		validate_wakeup(id, wakeup_target, prev);
	}
}

/**
 * @brief Test preemption
 *
 * @ingroup kernel_sched_tests
 */
ZTEST(suite_preempt, test_preempt)
{
	int priority;

	for (int i = 0; i < NUM_THREADS; i++) {
		k_sem_init(&worker_sems[i], 0, 1);

		if (worker_priorities[i] == METAIRQ) {
			priority = K_HIGHEST_THREAD_PRIO;
		} else if (worker_priorities[i] == COOP) {
			priority = K_HIGHEST_THREAD_PRIO
				+ CONFIG_NUM_METAIRQ_PRIORITIES;

			zassert_true(priority < K_PRIO_PREEMPT(0), "");
		} else {
			priority = K_LOWEST_APPLICATION_THREAD_PRIO - 1;

			zassert_true(priority >= K_PRIO_PREEMPT(0), "");
		}

		k_thread_create(&worker_threads[i],
				worker_stacks[i], STACK_SIZE,
				worker, INT_TO_POINTER(i), NULL, NULL,
				priority, 0, K_NO_WAIT);
	}

	k_thread_create(&manager_thread, manager_stack, STACK_SIZE,
			manager, NULL, NULL, NULL,
			K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_NO_WAIT);

	/* We don't control the priority of this thread so can't make
	 * it part of the test.  Just get out of the way until the
	 * test is done
	 */
	k_sem_take(&main_sem, K_FOREVER);

	/* unit test clean up */

	/* k_thread_abort() also works here.
	 * But join should be more graceful.
	 */
	k_thread_join(&manager_thread, K_FOREVER);

	/* worker threads have to be aborted.
	 * It is difficult to make them stop gracefully.
	 */
	for (int i = 0; i < NUM_THREADS; i++) {
		k_thread_abort(&worker_threads[i]);
	}

}

ZTEST_SUITE(suite_preempt, NULL, NULL, NULL, NULL, NULL);
