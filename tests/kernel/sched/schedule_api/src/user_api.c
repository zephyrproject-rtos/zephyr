/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_sched.h"
#include <zephyr/ztest.h>
#include <zephyr/irq_offload.h>
#include <kernel_internal.h>
#include <zephyr/ztest_error_hook.h>

struct k_thread user_thread;
K_SEM_DEFINE(user_sem, 0, 1);

ZTEST_BMEM volatile int thread_was_preempt;

#define THREAD_TEST_PRIORITY 0

K_THREAD_STACK_DEFINE(ustack, STACK_SIZE);

static void sleepy_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_sleep(K_FOREVER);
	k_sem_give(&user_sem);
}

/**
 * @brief Verify k_wakeup() wakes a sleeping thread from user mode.
 *
 * @details
 * A user-mode thread must be able to wake another thread that is blocked in
 * k_sleep(K_FOREVER) by calling k_wakeup(). The woken thread proceeds and
 * signals completion.
 *
 * Test steps:
 * - From a user thread, spawn a user thread that sleeps forever.
 * - Call k_wakeup() on it and wait for it to signal it resumed.
 *
 * Expected result:
 * - The sleeping thread is woken and runs to completion.
 *
 * @ingroup tests_kernel_sched
 *
 * @see k_wakeup()
 * @verifies ZEP-SRS-28-11
 */
ZTEST_USER(threads_scheduling, test_user_k_wakeup)
{
	k_tid_t tid = k_thread_create(&user_thread, ustack, STACK_SIZE, sleepy_thread,
			NULL, NULL, NULL,
			k_thread_priority_get(k_current_get()),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_yield(); /* Let thread run and start sleeping forever */
	k_wakeup(&user_thread);
	k_sem_take(&user_sem, K_FOREVER);
	k_thread_abort(tid);
}

static void preempt_test_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	thread_was_preempt = k_is_preempt_thread();
	k_sem_give(&user_sem);
}

/**
 * @brief Verify k_is_preempt_thread() reflects priority from user mode.
 *
 * @details
 * k_is_preempt_thread() must report false for a cooperative-priority thread and
 * true for a preemptible-priority thread, when queried from user mode. The test
 * spawns a worker at each priority class and checks the reported value.
 *
 * Test steps:
 * - Spawn a user worker at the (cooperative) caller priority; expect false.
 * - Spawn a user worker at a preemptible priority; expect true.
 *
 * Expected result:
 * - k_is_preempt_thread() is false for the cooperative thread and true for the
 *   preemptible one.
 *
 * @ingroup tests_kernel_sched
 *
 * @see k_is_preempt_thread()
 * @verifies ZEP-SRS-2-20
 */
ZTEST_USER(threads_scheduling, test_user_k_is_preempt)
{
	/* thread_was_preempt is volatile, and static analysis doesn't
	 * like to see it being tested inside zassert_true, because
	 * the read is treated as a "side effect" of an assertion
	 * (e.g. a read is significant for things like volatile MMIO
	 * addresses, and assertions may or may not be compiled, even
	 * though here in a test they always will be and in any case
	 * the value is a static variable above marked volatile for
	 * threadsafety).  Read it into a local variable first to
	 * evade the warning.
	 */
	int twp;

	k_tid_t tid = k_thread_create(&user_thread, ustack,
			STACK_SIZE, preempt_test_thread, NULL, NULL, NULL,
			k_thread_priority_get(k_current_get()),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_sem_take(&user_sem, K_FOREVER);

	twp = thread_was_preempt;
	zassert_false(twp, "unexpected return value");
	k_thread_abort(tid);

	tid = k_thread_create(&user_thread, ustack, STACK_SIZE, preempt_test_thread,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(1),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_sem_take(&user_sem, K_FOREVER);

	twp = thread_was_preempt;
	zassert_true(twp, "unexpected return value");
	k_thread_abort(tid);
}

/*
 * The tests below are userspace negative tests: they pass NULL (or an
 * out-of-range value) to an API to verify it triggers a fatal exception. They
 * only make sense under CONFIG_USERSPACE; on other configurations each test
 * skips. The guard lives inside the single test body (rather than a duplicate
 * stub definition) and around the fault-helper thread to avoid an unused
 * function warning.
 */
#ifdef CONFIG_USERSPACE
static void thread_suspend_init_null(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	ztest_set_fault_valid(true);
	k_thread_suspend(NULL);

	/* should not go here */
	ztest_test_fail();
}
#endif

/**
 * @brief Test k_thread_suspend() API
 *
 * @details Create a thread and set k_thread_suspend() input param to NULL
 * will trigger a fatal error. Skipped when CONFIG_USERSPACE is disabled.
 *
 * @ingroup tests_kernel_sched
 *
 * @see k_thread_suspend()
 * @verifies ZEP-SRS-1-3
 */
ZTEST_USER(threads_scheduling, test_k_thread_suspend_init_null)
{
#ifdef CONFIG_USERSPACE
	k_tid_t tid = k_thread_create(&user_thread, ustack, STACK_SIZE,
			thread_suspend_init_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
#else
	ztest_test_skip();
#endif
}

#ifdef CONFIG_USERSPACE
static void thread_resume_init_null(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	ztest_set_fault_valid(true);
	k_thread_resume(NULL);

	/* should not go here */
	ztest_test_fail();
}
#endif

/**
 * @brief Test k_thread_resume() API
 *
 * @details Create a thread and set k_thread_resume() input param to NULL
 * will trigger a fatal error. Skipped when CONFIG_USERSPACE is disabled.
 *
 * @ingroup tests_kernel_sched
 *
 * @see k_thread_resume()
 * @verifies ZEP-SRS-1-4
 */
ZTEST_USER(threads_scheduling, test_k_thread_resume_init_null)
{
#ifdef CONFIG_USERSPACE
	k_tid_t tid = k_thread_create(&user_thread, ustack, STACK_SIZE,
			thread_resume_init_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
#else
	ztest_test_skip();
#endif
}

#ifdef CONFIG_USERSPACE
static void thread_priority_get_init_null(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	ztest_set_fault_valid(true);
	k_thread_priority_get(NULL);

	/* should not go here */
	ztest_test_fail();
}
#endif

/**
 * @brief Test k_thread_priority_get() API
 *
 * @details Create a thread and set thread_k_thread_priority_get() param input to
 * NULL will trigger a fatal error. Skipped when CONFIG_USERSPACE is disabled.
 *
 * @ingroup tests_kernel_sched
 *
 * @see k_thread_priority_get()
 * @verifies ZEP-SRS-1-16
 */
ZTEST_USER(threads_scheduling, test_k_thread_priority_get_init_null)
{
#ifdef CONFIG_USERSPACE
	k_tid_t tid = k_thread_create(&user_thread, ustack, STACK_SIZE,
			thread_priority_get_init_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
#else
	ztest_test_skip();
#endif
}

#ifdef CONFIG_USERSPACE
static void thread_priority_set_init_null(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	ztest_set_fault_valid(true);
	k_thread_priority_set(NULL, 0);

	/* should not go here */
	ztest_test_fail();
}
#endif

/**
 * @brief Test k_thread_priority_set() with a NULL thread.
 *
 * @details Create a thread and set k_thread_priority_set() param input to
 * NULL will trigger a fatal error. Skipped when CONFIG_USERSPACE is disabled.
 *
 * @ingroup tests_kernel_sched
 *
 * @see k_thread_priority_set()
 * @verifies ZEP-SRS-1-2
 */
ZTEST_USER(threads_scheduling, test_k_thread_priority_set_init_null)
{
#ifdef CONFIG_USERSPACE
	k_tid_t tid = k_thread_create(&user_thread, ustack, STACK_SIZE,
			thread_priority_set_init_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
#else
	ztest_test_skip();
#endif
}

#ifdef CONFIG_USERSPACE
static void thread_priority_set_overmax(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	ztest_set_fault_valid(true);

	/* set valid priority value outside the priority range will invoke fatal error */
	k_thread_priority_set(k_current_get(), K_LOWEST_APPLICATION_THREAD_PRIO + 1);

	/* should not go here */
	ztest_test_fail();
}
#endif

/**
 * @brief Test k_thread_priority_set() with an out-of-range priority.
 *
 * @details Check input param range overmax in userspace test. Skipped when
 * CONFIG_USERSPACE is disabled.
 *
 * @ingroup tests_kernel_sched
 *
 * @see k_thread_priority_set()
 * @verifies ZEP-SRS-1-2
 */
ZTEST_USER(threads_scheduling, test_k_thread_priority_set_overmax)
{
#ifdef CONFIG_USERSPACE
	k_tid_t tid = k_thread_create(&user_thread, ustack, STACK_SIZE,
			thread_priority_set_overmax,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
#else
	ztest_test_skip();
#endif
}

#ifdef CONFIG_USERSPACE
static void thread_priority_set_upgrade(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	ztest_set_fault_valid(true);

	/* at first, set an valid priority */
	k_thread_priority_set(k_current_get(), THREAD_TEST_PRIORITY);
	/* it cannot upgraded thread priority in usermode */
	k_thread_priority_set(k_current_get(), THREAD_TEST_PRIORITY - 1);

	/* should not go here */
	ztest_test_fail();
}
#endif

/**
 * @brief Test that a user thread cannot raise its own priority.
 *
 * @details Check input param range fail in userspace test. Skipped when
 * CONFIG_USERSPACE is disabled.
 *
 * @ingroup tests_kernel_sched
 *
 * @see k_thread_priority_set()
 * @verifies ZEP-SRS-1-2
 */
ZTEST_USER(threads_scheduling, test_k_thread_priority_set_upgrade)
{
#ifdef CONFIG_USERSPACE
	k_tid_t tid = k_thread_create(&user_thread, ustack, STACK_SIZE,
			thread_priority_set_upgrade,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
#else
	ztest_test_skip();
#endif
}

#ifdef CONFIG_USERSPACE
static void thread_wakeup_init_null(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	ztest_set_fault_valid(true);
	k_wakeup(NULL);

	/* should not go here */
	ztest_test_fail();
}
#endif

/**
 * @brief Test k_wakeup() with a NULL thread.
 *
 * @details Create a thread and set k_wakeup() input param to NULL
 * will trigger a fatal error. Skipped when CONFIG_USERSPACE is disabled.
 *
 * @ingroup tests_kernel_sched
 *
 * @see k_wakeup()
 * @verifies ZEP-SRS-28-11
 */
ZTEST_USER(threads_scheduling, test_k_wakeup_init_null)
{
#ifdef CONFIG_USERSPACE
	k_tid_t tid = k_thread_create(&user_thread, ustack, STACK_SIZE,
			thread_wakeup_init_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
#else
	ztest_test_skip();
#endif
}
