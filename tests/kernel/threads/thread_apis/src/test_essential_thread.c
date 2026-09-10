/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

/* Internal APIs */
#include <kernel_internal.h>
#include <ksched.h>
#include <kthread.h>

struct k_thread kthread_thread;
struct k_thread kthread_thread1;

#define STACKSIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
K_THREAD_STACK_DEFINE(kthread_stack, STACKSIZE);
K_SEM_DEFINE(sync_sem, 0, 1);

static bool fatal_error_signaled;

static void thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	z_thread_essential_set(_current);

	if (z_is_thread_essential(_current)) {
		k_busy_wait(100);
	} else {
		zassert_unreachable("The thread is not set as essential");
	}

	z_thread_essential_clear(_current);
	zassert_false(z_is_thread_essential(_current),
		      "Essential flag of the thread is not cleared");

	k_sem_give(&sync_sem);
}

/**
 * @brief Verify that a thread can mark itself essential and clear it again.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * The essential flag is part of a thread's own state and can be turned on and
 * off at run time, not only at creation. The spawned thread sets the flag,
 * checks that it reads back as set, clears it and checks again, so what is
 * validated is the flag the kernel actually recorded. Clearing it also lets
 * the thread be aborted afterwards without panicking the kernel.
 *
 * Test steps:
 * - Create a thread that calls k_thread_essential_set() and confirms
 *   k_is_essential() reports it.
 * - Have the thread clear the flag and confirm it no longer reports.
 * - Signal the test thread, which then aborts the worker.
 *
 * Expected result:
 * - The flag reads back as set after being set and clear after being cleared,
 *   and the thread aborts without raising a fatal error.
 *
 * @see k_thread_essential_set()
 * @see k_thread_essential_clear()
 * @see k_is_essential()
 */
ZTEST(threads_lifecycle, test_thread_essential_set_clear)
{
	k_tid_t tid = k_thread_create(&kthread_thread, kthread_stack,
				      STACKSIZE, thread_entry, NULL,
				      NULL, NULL, K_PRIO_PREEMPT(0), 0,
				      K_NO_WAIT);

	k_sem_take(&sync_sem, K_FOREVER);
	k_thread_abort(tid);
}

void k_sys_fatal_error_handler(unsigned int reason,
				      const struct arch_esf *esf)
{
	ARG_UNUSED(esf);
	ARG_UNUSED(reason);

	fatal_error_signaled = true;
}

static void abort_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	if (z_is_thread_essential(_current)) {
		k_msleep(200);
	} else {
		zassert_unreachable("The thread is not set as essential");
	}

	zassert_true(false, "Should not reach this line");
}

static void abort_thread_self(void *p1, void *p2, void *p3)
{
	k_thread_abort(k_current_get());
	zassert_true(false, "Should not reach this line");
}

/**
 * @brief Verify that aborting an essential thread raises a fatal error.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * An essential thread is one the system cannot continue without, so the kernel
 * must treat its termination as a fatal system error rather than reclaiming it
 * quietly. The test installs a fatal error handler that records the event and
 * lets the system continue, so the abort can be observed instead of halting
 * the run.
 *
 * Test steps:
 * - Create a thread with the K_ESSENTIAL option and let it start.
 * - Abort it from the test thread.
 * - Check the flag recorded by the fatal error handler.
 *
 * Expected result:
 * - The kernel raises a fatal error for the aborted essential thread.
 *
 * @see K_ESSENTIAL
 * @see k_thread_abort()
 * @see k_sys_fatal_error_handler()
 */
ZTEST(threads_lifecycle, test_thread_essential_abort_panics)
{
	fatal_error_signaled = false;
	k_thread_create(&kthread_thread1, kthread_stack, STACKSIZE,
			abort_thread_entry,
			NULL, NULL, NULL, K_PRIO_PREEMPT(0), K_ESSENTIAL,
			K_NO_WAIT);

	k_msleep(100);
	k_thread_abort(&kthread_thread1);
	zassert_true(fatal_error_signaled, "fatal error was not signaled");
}

/**
 * @brief Verify that an essential thread aborting itself raises a fatal error.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * The same rule has to hold when the essential thread ends itself rather than
 * being aborted by someone else, which is the harder path: the panic is raised
 * on the very thread that is going away, so the architecture layer has to
 * unwind a thread that aborts inside its own fatal error handling.
 *
 * Test steps:
 * - Create a thread with the K_ESSENTIAL option whose entry point returns,
 *   ending the thread from within itself.
 * - Wait for the fatal error handler to record the event.
 *
 * Expected result:
 * - The kernel raises a fatal error for the self-terminating essential thread.
 *
 * @see K_ESSENTIAL
 * @see k_sys_fatal_error_handler()
 */
ZTEST(threads_lifecycle, test_thread_essential_abort_self_panics)
{
	/* This test case needs to be able to handle a k_panic() call
	 * that aborts the current thread inside of the panic handler
	 * itself.  That's putting a lot of strain on the arch layer
	 * to handle things that haven't traditionally been required.
	 * These ones aren't there yet.
	 *
	 * But run it for everyone else to catch regressions in the
	 * code we are actually trying to test.
	 */
	if (IS_ENABLED(CONFIG_X86) || IS_ENABLED(CONFIG_SPARC)) {
		ztest_test_skip();
	}

	fatal_error_signaled = false;
	k_thread_create(&kthread_thread1, kthread_stack, STACKSIZE,
			abort_thread_self,
			NULL, NULL, NULL, K_PRIO_PREEMPT(0), K_ESSENTIAL,
			K_NO_WAIT);

	k_msleep(100);
	zassert_true(fatal_error_signaled, "fatal error was not signaled");
}
