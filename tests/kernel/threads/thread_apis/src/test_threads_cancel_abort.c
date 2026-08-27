/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include "tests_thread_apis.h"

static ZTEST_BMEM int execute_flag;

K_SEM_DEFINE(sync_sema, 0, 1);
#define BLOCK_SIZE 64

static void thread_entry(void *p1, void *p2, void *p3)
{
	execute_flag = 1;
	k_msleep(100);
	execute_flag = 2;
}

static void thread_entry_abort(void *p1, void *p2, void *p3)
{
	/**TESTPOINT: abort current thread*/
	execute_flag = 1;
	k_thread_abort(k_current_get());
	CODE_UNREACHABLE;
	/*unreachable*/
	execute_flag = 2;
	zassert_true(1 == 0);
}
/**
 * @brief Verify that a thread can abort itself.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * A thread calling k_thread_abort() on itself must not return from the call:
 * it has to terminate at that point. The spawned thread raises a flag, aborts
 * itself, and would raise a second value on the line after the call, so the
 * flag distinguishes "ran and stopped there" from "ran past the abort".
 *
 * Test steps:
 * - Create a user thread that flags that it ran and then aborts itself.
 * - Sleep long enough for it to run.
 * - Read the flag back.
 *
 * Expected result:
 * - The thread ran and stopped inside the abort, never reaching the code
 *   after it.
 *
 * @see k_thread_abort()
 */
ZTEST_USER(threads_lifecycle, test_thread_abort_self)
{
	execute_flag = 0;
	k_thread_create(&tdata, tstack, STACK_SIZE, thread_entry_abort,
			NULL, NULL, NULL, 0, K_USER, K_NO_WAIT);
	k_msleep(100);
	/**TESTPOINT: spawned thread executed but abort itself*/
	zassert_true(execute_flag == 1);
}

/**
 * @brief Verify that a thread can be aborted by another thread.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * Aborting another thread has to work both before that thread has had a
 * chance to run and after it has started, since the two take different paths:
 * one removes a ready thread that was never scheduled, the other terminates
 * one that is already executing. Both are covered here from user mode.
 *
 * Test steps:
 * - Create a user thread and abort it immediately, before it can run.
 * - Sleep past the point it would have run and check its flag.
 * - Create a second user thread, let it start, then abort it.
 *
 * Expected result:
 * - The thread aborted before starting never runs, and the one aborted after
 *   starting is terminated.
 *
 * @see k_thread_abort()
 */
ZTEST_USER(threads_lifecycle, test_thread_abort_others)
{
	execute_flag = 0;
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_entry, NULL, NULL, NULL,
				      0, K_USER, K_NO_WAIT);

	k_thread_abort(tid);
	k_msleep(100);
	/**TESTPOINT: check not-started thread is aborted*/
	zassert_true(execute_flag == 0);

	tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			      thread_entry, NULL, NULL, NULL,
			      0, K_USER, K_NO_WAIT);
	k_msleep(50);
	k_thread_abort(tid);
	/**TESTPOINT: check running thread is aborted*/
	zassert_true(execute_flag == 1);
	k_msleep(1000);
	zassert_true(execute_flag == 1);
}

/**
 * @brief Verify that aborting an already terminated thread is harmless.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * A thread that has already been aborted may still be referenced by code that
 * does not know it is gone, so a second k_thread_abort() on it has to be
 * accepted quietly rather than corrupting the scheduler or faulting.
 *
 * Test steps:
 * - Create a thread and abort it.
 * - Call k_thread_abort() on the same thread again, twice.
 *
 * Expected result:
 * - The repeated aborts complete without error and the system keeps running.
 *
 * @see k_thread_abort()
 */
ZTEST(threads_lifecycle_1cpu, test_thread_abort_repeat)
{
	execute_flag = 0;
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_entry, NULL, NULL, NULL,
				      0, K_USER, K_NO_WAIT);

	k_thread_abort(tid);
	k_msleep(100);
	k_thread_abort(tid);
	k_msleep(100);
	k_thread_abort(tid);
	/* If no fault occurred till now. The test case passed. */
	ztest_test_pass();
}

static void delayed_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	execute_flag = 1;

	zassert_unreachable("Delayed thread shouldn't be executed");
}

/**
 * @ingroup kernel_thread_tests
 * @brief Test abort on delayed thread before it has started
 * execution
 *
 * @details
 * A thread that is still waiting out its start delay is not yet on any ready
 * queue, so aborting it has to cancel the pending start rather than leave a
 * timeout that would launch a dead thread later. Sleeping past the original
 * deadline is what makes a surviving timeout visible.
 *
 * Test steps:
 * - Create a thread with a 100 ms start delay.
 * - Abort it while it is still waiting to start.
 * - Sleep past the original start deadline.
 * - Check the flag the thread would have set had it run.
 *
 * Expected result:
 * - The thread never runs, so its flag stays clear.
 *
 * @see k_thread_abort()
 */
ZTEST(threads_lifecycle_1cpu, test_thread_abort_delayed)
{
	int current_prio = k_thread_priority_get(k_current_get());

	execute_flag = 0;
	/* Make current thread preemptive */
	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(2));

	/* Create a preemptive thread of higher priority than
	 * current thread
	 */
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      delayed_thread_entry, NULL, NULL, NULL,
				      K_PRIO_PREEMPT(1), 0, K_MSEC(100));

	/* Give up CPU */
	k_msleep(50);

	/* Test point: check if thread delayed for 100ms has not started*/
	zassert_true(execute_flag == 0, "Delayed thread created is not"
		     " put to wait queue");

	k_thread_abort(tid);

	/* Sleep past the thread's original 100ms start deadline. A working
	 * abort keeps execute_flag at 0; a broken abort would let the thread
	 * start and set it to 1.
	 */
	k_msleep(100);

	/* Test point: Test abort of thread before its execution*/
	zassert_true(execute_flag == 0, "Delayed thread has executed"
		     " after its start deadline despite being aborted");

	/* Restore the priority */
	k_thread_priority_set(k_current_get(), current_prio);
}

static volatile bool isr_finished;

static void offload_func(const void *param)
{
	struct k_thread *t = (struct k_thread *)param;

	k_thread_abort(t);

	/* Thread memory is unused now, validate that we can clobber it. */
	if (!IS_ENABLED(CONFIG_ARCH_POSIX)) {
		memset(t, 0, sizeof(*t));
	}

	/* k_thread_abort() in an isr shouldn't affect the ISR's execution */
	isr_finished = true;
}

static void entry_abort_isr(void *p1, void *p2, void *p3)
{
	/* Simulate taking an interrupt which kills this thread */
	irq_offload(offload_func, k_current_get());

	printk("shouldn't see this, thread should have been killed");
	ztest_test_fail();
}

extern struct k_sem offload_sem;

/**
 * @brief Verify that a thread can be aborted from an ISR interrupting it.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * k_thread_abort() is callable from interrupt context, including on the very
 * thread the ISR interrupted. The kernel has to let the ISR run to completion
 * and only then drop the aborted thread, rather than switching away inside
 * the handler. The ISR sets a flag as its last act, so the test can tell that
 * it finished as well as that the thread died.
 *
 * Test steps:
 * - Spawn a thread that enters an ISR and aborts itself from there.
 * - Join the spawned thread.
 * - Check the flag the ISR sets before returning.
 *
 * Expected result:
 * - The ISR completes and the interrupted thread is terminated.
 *
 * @see k_thread_abort()
 * @see irq_offload()
 */
ZTEST(threads_lifecycle, test_thread_abort_from_isr)
{
	isr_finished = false;
	k_thread_create(&tdata, tstack, STACK_SIZE, entry_abort_isr,
			NULL, NULL, NULL, 0, 0, K_NO_WAIT);


	k_thread_join(&tdata, K_FOREVER);
	zassert_true(isr_finished, "ISR did not complete");

	/* Thread struct was cleared after the abort, make sure it is
	 * still clear (i.e. that the arch layer didn't write to it
	 * during interrupt exit).  Doesn't work on posix, which needs
	 * the thread struct for its swap code.
	 */
	uint8_t *p = (uint8_t *)&tdata;

	if (!IS_ENABLED(CONFIG_ARCH_POSIX)) {
		for (int i = 0; i < sizeof(tdata); i++) {
			zassert_true(p[i] == 0, "Free memory write to aborted thread");
		}
	}

	/* Notice: Recover back the offload_sem: This is use for releasing
	 * offload_sem which might be held when thread aborts itself in ISR
	 * context, it will cause irq_offload cannot be used again.
	 */
	k_sem_give(&offload_sem);
}

/* use for sync thread start */
static struct k_sem sem_abort;

static void entry_aborted_thread(void *p1, void *p2, void *p3)
{
	k_sem_give(&sem_abort);

	/* wait for being aborted */
	while (1) {
		k_sleep(K_MSEC(1));
	}
	zassert_unreachable("should not reach here");
}

/**
 * @brief Verify that a thread can be aborted from an ISR on another thread.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * The counterpart of aborting the interrupted thread: here the ISR runs on
 * the test thread and aborts a different, already running thread. That target
 * may be executing on another CPU or merely be ready, so the abort has to
 * complete from interrupt context without waiting on the target to reach a
 * scheduling point.
 *
 * Test steps:
 * - Spawn a thread and let it start running.
 * - From the test thread, enter an ISR and abort the spawned thread there.
 * - Join the spawned thread and check the flag the ISR sets.
 *
 * Expected result:
 * - The ISR completes and the target thread is terminated.
 *
 * @see k_thread_abort()
 * @see irq_offload()
 */
ZTEST(threads_lifecycle, test_thread_abort_from_isr_not_self)
{
	k_tid_t tid;

	isr_finished = false;
	k_sem_init(&sem_abort, 0, 1);

	tid = k_thread_create(&tdata, tstack, STACK_SIZE, entry_aborted_thread,
			NULL, NULL, NULL, 0, 0, K_NO_WAIT);

	/* wait for thread started */
	k_sem_take(&sem_abort, K_FOREVER);

	/* Simulate taking an interrupt which kills spawn thread */
	irq_offload(offload_func, (void *)tid);

	zassert_true(isr_finished, "ISR did not complete");
}
