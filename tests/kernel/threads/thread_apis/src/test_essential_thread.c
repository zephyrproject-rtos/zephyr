/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>

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
 * @brief Test to validate essential flag set/clear
 *
 * @ingroup kernel_thread_tests
 *
 * @see #K_ESSENTIAL(x)
 */
ZTEST(threads_lifecycle, test_essential_thread_operation)
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
 * @brief Abort an essential thread
 *
 * @details The kernel shall raise a fatal system error if an essential thread
 *          aborts, implement k_sys_fatal_error_handler to handle this error.
 *
 * @ingroup kernel_thread_tests
 *
 * @see #K_ESSENTIAL(x)
 */

ZTEST(threads_lifecycle, test_essential_thread_abort)
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

ZTEST(threads_lifecycle, test_essential_thread_abort_self)
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
	if (IS_ENABLED(CONFIG_RISCV) || IS_ENABLED(CONFIG_X86) || IS_ENABLED(CONFIG_SPARC)) {
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
