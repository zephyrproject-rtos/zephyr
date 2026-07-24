/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Aborting an essential thread raises a kernel panic. The context doing the
 * aborting is not necessarily the essential thread itself: it may be another
 * thread, or an ISR (this is how a work queue handler that overruns its
 * CONFIG_WORKQUEUE_WORK_TIMEOUT is dealt with). These tests check that the
 * fatal error is attributed to the essential thread that was aborted, and in
 * particular that it is that thread, and not the running one, which is killed
 * when k_sys_fatal_error_handler() returns.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

#define STACK_SIZE      (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define WORK_TIMEOUT_MS 100

static K_THREAD_STACK_DEFINE(essential_stack, STACK_SIZE);
static struct k_thread essential_thread;

static K_KERNEL_STACK_DEFINE(workq_stack, STACK_SIZE);
static struct k_work_q workq;

static struct k_sem thread_started;

/* Written by the fatal error handler, read by the test thread. */
static volatile unsigned int fatal_reason;
static volatile int fatal_count;

/* Called by the ztest fatal error handler for an expected error. Returning
 * from it lets z_fatal_error() proceed to kill the faulting thread, which is
 * exactly the behaviour under test.
 */
void ztest_post_fatal_error_hook(unsigned int reason, const struct arch_esf *pEsf)
{
	ARG_UNUSED(pEsf);

	fatal_reason = reason;
	fatal_count++;
}

static void essential_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_sem_give(&thread_started);
	k_sleep(K_FOREVER);
}

static k_tid_t start_essential_thread(void)
{
	k_tid_t tid;

	k_sem_init(&thread_started, 0, 1);

	tid = k_thread_create(&essential_thread, essential_stack,
			      K_THREAD_STACK_SIZEOF(essential_stack),
			      essential_entry, NULL, NULL, NULL,
			      K_PRIO_PREEMPT(1), K_ESSENTIAL, K_NO_WAIT);
	k_thread_name_set(tid, "essential");

	zassert_ok(k_sem_take(&thread_started, K_MSEC(500)),
		   "essential thread did not start");

	return tid;
}

/* Runs from the sys clock ISR. */
static void abort_from_isr(struct k_timer *timer)
{
	k_tid_t tid = k_timer_user_data_get(timer);

	/* Mark the panic raised below as expected. Called from an ISR, so the
	 * ztest hook records it as an ISR fault rather than a thread one.
	 */
	ztest_set_fault_valid(true);
	k_thread_abort(tid);
}

static K_TIMER_DEFINE(abort_timer, abort_from_isr, NULL);

/* Runs from the sys clock ISR, ahead of the work timeout it arms the hook for. */
static void mark_fault_expected(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	ztest_set_fault_valid(true);
}

static K_TIMER_DEFINE(mark_timer, mark_fault_expected, NULL);

static void hung_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	/* Never returns: the work queue's work timeout expires on us. */
	k_sleep(K_FOREVER);
}

static K_WORK_DEFINE(hung_work, hung_work_handler);

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	fatal_reason = 0;
	fatal_count = 0;
}

ZTEST_SUITE(fatal_essential_thread, NULL, NULL, before, NULL, NULL);

/**
 * @brief Kernel essential thread fatal error tests
 * @defgroup tests_kernel_fatal_essential_thread Essential thread panic
 * @ingroup all_tests
 * @{
 */

/**
 * @brief Verify that aborting an essential thread from another thread panics,
 *        and kills the essential thread rather than the aborting one.
 *
 * @details
 * Aborting an essential thread is a fatal condition, and the fatal error must
 * be attributed to the essential thread that was aborted, not to the thread
 * that requested the abort. When k_sys_fatal_error_handler() returns, the
 * kernel kills the faulting thread; that must be the essential thread, leaving
 * the aborting thread running.
 *
 * Test steps:
 * - Start a thread with the K_ESSENTIAL option and wait for it to run.
 * - Arm the ztest fatal hook and call k_thread_abort() on it from the test
 *   thread.
 *
 * Expected result:
 * - k_sys_fatal_error_handler() is invoked exactly once, with
 *   K_ERR_KERNEL_PANIC.
 * - The essential thread is dead.
 * - The aborting (test) thread survives and completes the test.
 *
 * @see k_thread_abort()
 * @see k_sys_fatal_error_handler()
 */
ZTEST(fatal_essential_thread, test_essential_thread_abort_from_thread_panics)
{
	k_tid_t tid = start_essential_thread();

	ztest_set_fault_valid(true);
	k_thread_abort(tid);

	/* Only reached if the fatal path killed the aborted essential thread
	 * rather than the running one. Were it to kill the running thread, this
	 * test thread would die here and the test would never complete.
	 */
	zassert_equal(fatal_count, 1, "fatal handler ran %d times, expected 1", fatal_count);
	zassert_equal(fatal_reason, K_ERR_KERNEL_PANIC, "unexpected fatal reason %u",
		      fatal_reason);
	zassert_ok(k_thread_join(tid, K_NO_WAIT), "essential thread is still alive");
}

/**
 * @brief Verify that aborting an essential thread from an ISR panics, and does
 *        not kill the interrupted thread.
 *
 * @details
 * When an essential thread is aborted from interrupt context there is no
 * faulting thread running: the thread the ISR interrupted is an unrelated
 * bystander. The panic must still be attributed to the essential thread, and
 * it must be that thread, not the interrupted one, which the kernel kills when
 * k_sys_fatal_error_handler() returns.
 *
 * Test steps:
 * - Start a thread with the K_ESSENTIAL option and wait for it to run.
 * - Start a timer whose expiry function aborts the essential thread from ISR
 *   context.
 * - Busy-wait in the test thread so that it is the thread the timer ISR
 *   interrupts.
 *
 * Expected result:
 * - k_sys_fatal_error_handler() is invoked exactly once, with
 *   K_ERR_KERNEL_PANIC.
 * - The essential thread is dead.
 * - The interrupted (test) thread is still alive.
 *
 * @see k_thread_abort()
 * @see k_sys_fatal_error_handler()
 */
ZTEST(fatal_essential_thread, test_essential_thread_abort_from_isr_panics)
{
	k_tid_t tid = start_essential_thread();

	k_timer_user_data_set(&abort_timer, tid);
	k_timer_start(&abort_timer, K_MSEC(50), K_NO_WAIT);

	/* Busy-wait rather than sleep, so that this thread is the one the timer
	 * ISR interrupts, and hence the one a misattributed panic would kill.
	 */
	k_busy_wait(150 * USEC_PER_MSEC);

	zassert_equal(fatal_count, 1, "fatal handler ran %d times, expected 1", fatal_count);
	zassert_equal(fatal_reason, K_ERR_KERNEL_PANIC, "unexpected fatal reason %u",
		      fatal_reason);
	zassert_ok(k_thread_join(tid, K_NO_WAIT), "essential thread is still alive");
	zassert_true(k_thread_join(k_current_get(), K_NO_WAIT) != 0,
		     "interrupted thread was killed");
}

/**
 * @brief Verify that a work handler overrunning the work timeout of an
 *        essential work queue panics and blames the work queue thread.
 *
 * @details
 * With CONFIG_WORKQUEUE_WORK_TIMEOUT, a work item whose handler runs longer
 * than the queue's work_timeout_ms has its work queue thread aborted from the
 * sys clock ISR. When that queue is essential, the abort is fatal, which is how
 * an application detects a hung work handler: through the regular
 * k_sys_fatal_error_handler(), where it can reboot (CONFIG_RESET_ON_FATAL_ERROR)
 * or run its own recovery. The panic must name the work queue thread, not the
 * thread the sys clock ISR happened to interrupt.
 *
 * Test steps:
 * - Start an essential work queue with a finite work_timeout_ms.
 * - Submit a work item whose handler never returns.
 * - Wait for the work timeout to expire.
 *
 * Expected result:
 * - k_sys_fatal_error_handler() is invoked exactly once, with
 *   K_ERR_KERNEL_PANIC.
 * - The work queue thread is dead.
 *
 * @see k_work_queue_start()
 * @see k_sys_fatal_error_handler()
 */
ZTEST(fatal_essential_thread, test_workq_work_timeout_panics_essential_queue)
{
	const struct k_work_queue_config cfg = {
		.name = "hung_wq",
		.essential = true,
		.work_timeout_ms = WORK_TIMEOUT_MS,
	};

	Z_TEST_SKIP_IFNDEF(CONFIG_WORKQUEUE_WORK_TIMEOUT);

	k_work_queue_start(&workq, workq_stack, K_KERNEL_STACK_SIZEOF(workq_stack),
			   K_PRIO_PREEMPT(1), &cfg);

	/* The panic is raised from the sys clock ISR when the work timeout
	 * expires, so arm the ztest hook from an ISR too, ahead of it.
	 */
	k_timer_start(&mark_timer, K_MSEC(WORK_TIMEOUT_MS / 2), K_NO_WAIT);

	zassert_equal(k_work_submit_to_queue(&workq, &hung_work), 1);

	zassert_ok(k_thread_join(workq.thread_id, K_MSEC(WORK_TIMEOUT_MS * 5)),
		   "work queue thread was not killed by the work timeout");
	zassert_equal(fatal_count, 1, "fatal handler ran %d times, expected 1", fatal_count);
	zassert_equal(fatal_reason, K_ERR_KERNEL_PANIC, "unexpected fatal reason %u",
		      fatal_reason);
}

/**
 * @}
 */
