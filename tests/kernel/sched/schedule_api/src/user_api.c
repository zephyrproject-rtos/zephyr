/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_sched.h"
#include <ztest.h>
#include <zephyr/irq_offload.h>
#include <kernel_internal.h>
#include <ztest_error_hook.h>

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

void test_user_k_wakeup(void)
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

void test_user_k_is_preempt(void)
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

/**
 * userspace negative test: take NULL as input param to verify
 * the api will trigger a fatal exception
 */
#ifdef CONFIG_USERSPACE
static void thread_suspend_init_null(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);
	k_thread_suspend(NULL);

	/* should not go here */
	ztest_test_fail();
}

/**
 * @brief Test k_thread_suspend() API
 *
 * @details Create a thread and set k_thread_suspend() input param to NULL
 * will trigger a fatal error.
 *
 * @ingroup kernel_sched_tests
 *
 * @see k_thread_suspend()
 */
void test_k_thread_suspend_init_null(void)
{
	k_tid_t tid = k_thread_create(&user_thread, ustack, STACK_SIZE,
			(k_thread_entry_t)thread_suspend_init_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}
#else
void test_k_thread_suspend_init_null(void)
{
	ztest_test_skip();
}
#endif

#ifdef CONFIG_USERSPACE
static void thread_resume_init_null(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);
	k_thread_resume(NULL);

	/* should not go here */
	ztest_test_fail();
}

/**
 * @brief Test k_thread_resume() API
 *
 * @details Create a thread and set k_thread_resume() input param to NULL
 * will trigger a fatal error.
 *
 * @ingroup kernel_sched_tests
 *
 * @see k_thread_resume()
 */
void test_k_thread_resume_init_null(void)
{
	k_tid_t tid = k_thread_create(&user_thread, ustack, STACK_SIZE,
			(k_thread_entry_t)thread_resume_init_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}
#else
void test_k_thread_resume_init_null(void)
{
	ztest_test_skip();
}
#endif

#ifdef CONFIG_USERSPACE
static void thread_priority_get_init_null(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);
	k_thread_priority_get(NULL);

	/* should not go here */
	ztest_test_fail();
}

/**
 * @brief Test k_thread_priority_get() API
 *
 * @details Create a thread and set thread_k_thread_priority_get() param input to
 * NULL will trigger a fatal error.
 *
 * @ingroup kernel_sched_tests
 *
 * @see thread_k_thread_priority_get()
 */
void test_k_thread_priority_get_init_null(void)
{
	k_tid_t tid = k_thread_create(&user_thread, ustack, STACK_SIZE,
			(k_thread_entry_t)thread_priority_get_init_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}
#else
void test_k_thread_priority_get_init_null(void)
{
	ztest_test_skip();
}
#endif

#ifdef CONFIG_USERSPACE
static void thread_priority_set_init_null(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);
	k_thread_priority_set(NULL, 0);

	/* should not go here */
	ztest_test_fail();
}

/**
 * @brief Test k_thread_priority_set() API
 *
 * @details Create a thread and set k_thread_priority_set() param input to
 * NULL will trigger a fatal error.
 *
 * @ingroup kernel_sched_tests
 *
 * @see k_thread_priority_set()
 */
void test_k_thread_priority_set_init_null(void)
{
	k_tid_t tid = k_thread_create(&user_thread, ustack, STACK_SIZE,
			(k_thread_entry_t)thread_priority_set_init_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}
#else
void test_k_thread_priority_set_init_null(void)
{
	ztest_test_skip();
}
#endif

#ifdef CONFIG_USERSPACE
static void thread_priority_set_overmax(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);

	/* set valid priority value outside the priority range will invoke fatal error */
	k_thread_priority_set(k_current_get(), K_LOWEST_APPLICATION_THREAD_PRIO + 1);

	/* should not go here */
	ztest_test_fail();
}

/**
 * @brief Test k_thread_priority_set() API
 *
 * @details Check input param range overmax in userspace test.
 *
 * @ingroup kernel_sched_tests
 *
 * @see k_thread_priority_set()
 */
void test_k_thread_priority_set_overmax(void)
{
	k_tid_t tid = k_thread_create(&user_thread, ustack, STACK_SIZE,
			(k_thread_entry_t)thread_priority_set_overmax,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}
#else
void test_k_thread_priority_set_overmax(void)
{
	ztest_test_skip();
}
#endif

#ifdef CONFIG_USERSPACE
static void thread_priority_set_upgrade(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);

	/* at first, set an valid priority */
	k_thread_priority_set(k_current_get(), THREAD_TEST_PRIORITY);
	/* it cannot upgraded thread priority in usermode */
	k_thread_priority_set(k_current_get(), THREAD_TEST_PRIORITY - 1);

	/* should not go here */
	ztest_test_fail();
}

/**
 * @brief Test k_thread_priority_set() API
 *
 * @details Check input param range fail in userspace test.
 *
 * @ingroup kernel_sched_tests
 *
 * @see k_thread_priority_set()
 */
void test_k_thread_priority_set_upgrade(void)
{
	k_tid_t tid = k_thread_create(&user_thread, ustack, STACK_SIZE,
			(k_thread_entry_t)thread_priority_set_upgrade,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}
#else
void test_k_thread_priority_set_upgrade(void)
{
	ztest_test_skip();
}
#endif

#ifdef CONFIG_USERSPACE
static void thread_wakeup_init_null(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);
	k_wakeup(NULL);

	/* should not go here */
	ztest_test_fail();
}

/**
 * @brief Test k_wakeup() API
 *
 * @details Create a thread and set k_wakeup() input param to NULL
 * will trigger a fatal error
 *
 * @ingroup kernel_sched_tests
 *
 * @see k_wakeup()
 */
void test_k_wakeup_init_null(void)
{
	k_tid_t tid = k_thread_create(&user_thread, ustack, STACK_SIZE,
			(k_thread_entry_t)thread_wakeup_init_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}
#else
void test_k_wakeup_init_null(void)
{
	ztest_test_skip();
}
#endif
