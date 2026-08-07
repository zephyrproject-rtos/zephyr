/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/ztest.h>
#include <zephyr/irq_offload.h>
#include <zephyr/ztest_error_hook.h>

#define STACK_SIZE     (512 + CONFIG_TEST_EXTRA_STACK_SIZE)

#define PRIO_WAIT (CONFIG_ZTEST_THREAD_PRIORITY)
#define PRIO_WAKE (CONFIG_ZTEST_THREAD_PRIORITY)

K_THREAD_STACK_DEFINE(stack_1, STACK_SIZE);
K_THREAD_STACK_DEFINE(condvar_wake_stack, STACK_SIZE);

struct k_thread condvar_tid;
struct k_thread condvar_wake_tid;

struct k_condvar simple_condvar;
K_MUTEX_DEFINE(test_mutex);

#define TOTAL_THREADS_WAITING (3)
#define TCOUNT 10
#define COUNT_LIMIT 12

ZTEST_BMEM int woken;
ZTEST_BMEM int timeout;
ZTEST_BMEM int count;

/* Shared state for the additional coverage tests further down. */
ZTEST_BMEM int woken_count;
ZTEST_BMEM int wake_order[TOTAL_THREADS_WAITING];
ZTEST_BMEM int wake_idx;
ZTEST_BMEM int contender_ret;

/* Statically initialized condition variable (no k_condvar_init() call). */
K_CONDVAR_DEFINE(static_condvar);

/* Mutex used to exercise the recursive-lock behaviour of k_condvar_wait(). */
struct k_mutex recursive_mtx;

struct k_condvar multiple_condvar[TOTAL_THREADS_WAITING];

struct k_thread multiple_tid[TOTAL_THREADS_WAITING];
struct k_thread multiple_wake_tid[TOTAL_THREADS_WAITING];
K_THREAD_STACK_ARRAY_DEFINE(multiple_stack,
		TOTAL_THREADS_WAITING, STACK_SIZE);
K_THREAD_STACK_ARRAY_DEFINE(multiple_wake_stack,
		TOTAL_THREADS_WAITING, STACK_SIZE);


/******************************************************************************/
/* Helper functions */
void condvar_isr_wake(const void *condvar)
{
	k_condvar_signal((struct k_condvar *)condvar);
}

void condvar_wake_from_isr(struct k_condvar *condvar)
{
	irq_offload(condvar_isr_wake, (const void *)condvar);
}

/* test condvar wait, no condvar wake */
void condvar_wait_task(void *p1, void *p2, void *p3)
{
	int32_t ret_value;
	k_ticks_t time_val = *(int *)p1;

	k_condvar_init(&simple_condvar);
	zassert_true(time_val >= (int)K_TICKS_FOREVER,
		     "invalid timeout parameter");

	k_mutex_lock(&test_mutex, K_FOREVER);
	ret_value = k_condvar_wait(&simple_condvar, &test_mutex, K_TICKS(time_val));

	switch (time_val) {
	case K_TICKS_FOREVER:
		/*
		 * With K_FOREVER and no waker, k_condvar_wait() must block
		 * indefinitely. Reaching here means it returned at all, which
		 * can only be a spurious wakeup -- fail unconditionally. In the
		 * expected case the test aborts this thread while it is still
		 * blocked, so this code is never reached.
		 */
		zassert_unreachable(
			"k_condvar_wait(K_FOREVER) returned without a signal (ret %d)",
			ret_value);
		break;
	case 0:
		zassert_true(ret_value == -EAGAIN,
		     "k_condvar_wait failed.");
		break;
	default:
		zassert_true(ret_value == -EAGAIN,
		     "k_condvar_wait failed.: %d", ret_value);
		break;
	}

	k_mutex_unlock(&test_mutex);

}

void condvar_wake_task(void *p1, void *p2, void *p3)
{
	int32_t ret_value;

	ret_value = k_condvar_signal(&simple_condvar);
	zassert_equal(ret_value, 0,
		"k_condvar_wake failed. (%d!=%d)", ret_value, 0);
}

void condvar_wake_multiple(void *p1, void *p2, void *p3)
{
	int32_t ret_value;
	int woken_num = *(int *)p1;

	ret_value = k_condvar_broadcast(&simple_condvar);
	zassert_true(ret_value == woken_num,
		"k_condvar_wake failed. (%d!=%d)", ret_value, woken_num);
}

void condvar_wait_wake_task(void *p1, void *p2, void *p3)
{
	int32_t ret_value;
	int time_val = *(int *)p1;

	zassert_true(time_val >= (int)K_TICKS_FOREVER, "invalid timeout parameter");
	k_mutex_lock(&test_mutex, K_FOREVER);
	ret_value = k_condvar_wait(&simple_condvar, &test_mutex, K_TICKS(time_val));

	switch (time_val) {
	case K_TICKS_FOREVER:
		zassert_true(ret_value == 0,
		     "k_condvar_wait failed.");
		break;
	case 0:
		zassert_true(ret_value == -EAGAIN,
		     "k_condvar_wait failed.");
		break;
	default:
		zassert_true(ret_value == 0,
		     "k_condvar_wait failed.");
		break;
	}

	k_mutex_unlock(&test_mutex);
}
/**
 * @brief Condition variable API tests
 * @defgroup kernel_condvar_tests Condition Variables
 * @ingroup all_tests
 * @{
 */

/**
 * @brief Verify a thread blocked forever is released by k_condvar_signal().
 *
 * @details
 * A helper thread locks a mutex and calls k_condvar_wait() with a K_FOREVER
 * timeout, so it blocks indefinitely. A second thread then calls
 * k_condvar_signal() on the same condition variable. The test verifies that the
 * signal wakes the blocked waiter, which returns 0 (woken, not timed out).
 *
 * Test steps:
 * - Initialize the condition variable.
 * - Start a waiter thread that calls k_condvar_wait(..., K_FOREVER).
 * - Start a waker thread that calls k_condvar_signal().
 *
 * Expected result:
 * - k_condvar_wait() in the waiter returns 0 (verified in
 *   condvar_wait_wake_task()).
 * - k_condvar_signal() returns 0.
 *
 * @see k_condvar_wait()
 * @see k_condvar_signal()
 */
ZTEST_USER(condvar_tests, test_condvar_wait_forever_wake)
{
	woken = 1;
	timeout = K_TICKS_FOREVER;

	k_condvar_init(&simple_condvar);
	k_thread_create(&condvar_tid, stack_1, STACK_SIZE,
			condvar_wait_wake_task, &timeout, NULL, NULL,
			PRIO_WAIT, K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	/* giving time for the condvar_wait_wake_task to execute */
	k_yield();

	k_thread_create(&condvar_wake_tid, condvar_wake_stack, STACK_SIZE,
			condvar_wake_task, &woken, NULL, NULL,
			PRIO_WAKE, K_USER | K_INHERIT_PERMS, K_MSEC(1));

	/* giving time for the condvar_wake_task
	 * and condvar_wait_wake_task to execute
	 */
	k_yield();

	k_thread_abort(&condvar_wake_tid);
	k_thread_abort(&condvar_tid);
}

/**
 * @brief Verify a finite-timeout waiter is woken by a signal before it times out.
 *
 * @details
 * A helper thread calls k_condvar_wait() with a 100 ms timeout and a second
 * thread signals the condition variable immediately. The signal arrives before
 * the timeout expires, so the waiter must return 0 (woken), not -EAGAIN.
 *
 * Expected result:
 * - k_condvar_wait() returns 0 (verified in condvar_wait_wake_task()).
 * - k_condvar_signal() returns 0.
 *
 * @see k_condvar_wait()
 * @see k_condvar_signal()
 */
ZTEST_USER(condvar_tests, test_condvar_wake_before_timeout)
{
	woken = 1;
	timeout = k_ms_to_ticks_ceil32(100);

	k_thread_create(&condvar_tid, stack_1, STACK_SIZE,
			condvar_wait_wake_task, &timeout, NULL, NULL,
			PRIO_WAIT, K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	/* giving time for the condvar_wait_wake_task to execute */
	k_yield();

	k_thread_create(&condvar_wake_tid, condvar_wake_stack, STACK_SIZE,
			condvar_wake_task, &woken, NULL, NULL,
			PRIO_WAKE, K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	/*
	 * giving time for the condvar_wake_task
	 * and condvar_wait_wake_task to execute
	 */
	k_yield();


	k_thread_abort(&condvar_wake_tid);
	k_thread_abort(&condvar_tid);
}

/**
 * @brief Verify k_condvar_wait() returns -EAGAIN when its timeout expires.
 *
 * @details
 * A helper thread calls k_condvar_wait() with a 50 ms timeout and no thread ever
 * signals the condition variable. The test verifies the wait fails with -EAGAIN
 * once the timeout elapses.
 *
 * Expected result:
 * - k_condvar_wait() returns -EAGAIN (verified in condvar_wait_task()).
 *
 * @see k_condvar_wait()
 */
ZTEST_USER(condvar_tests, test_condvar_wait_timeout)
{
	timeout = k_ms_to_ticks_ceil32(50);

	k_thread_create(&condvar_tid, stack_1, STACK_SIZE,
			condvar_wait_task, &timeout, NULL, NULL,
			PRIO_WAIT, K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	/* giving time for the condvar_wait_task to execute */
	k_sleep(K_MSEC(100));

	k_thread_abort(&condvar_tid);
}


/**
 * @brief Verify a K_FOREVER waiter stays blocked when no signal is sent.
 *
 * @details
 * A helper thread calls k_condvar_wait() with K_FOREVER and no thread ever
 * signals the condition variable. The waiter must remain blocked. The test
 * confirms this explicitly by joining the helper with a finite deadline: the
 * join must time out, proving the thread is still alive and blocked. The waiter
 * is then aborted.
 *
 * Test steps:
 * - Start a helper thread that calls k_condvar_wait(..., K_FOREVER).
 * - k_thread_join() the helper with a finite deadline.
 *
 * Expected result:
 * - k_thread_join() returns -EAGAIN (the deadline expires while the helper is
 *   still blocked). A return of 0 would mean a spurious wakeup let the helper
 *   terminate, which condvar_wait_task() also catches with zassert_unreachable().
 *
 * @see k_condvar_wait()
 * @see k_thread_join()
 */
ZTEST_USER(condvar_tests, test_condvar_wait_forever)
{
	int ret;

	timeout = K_TICKS_FOREVER;

	k_thread_create(&condvar_tid, stack_1, STACK_SIZE,
			condvar_wait_task, &timeout, NULL, NULL,
			PRIO_WAIT, K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	/*
	 * No thread ever signals the condition variable, so the waiter must
	 * stay blocked. Joining with a deadline must time out (-EAGAIN); a
	 * return of 0 would mean k_condvar_wait() returned spuriously and the
	 * helper terminated.
	 */
	ret = k_thread_join(&condvar_tid, K_MSEC(100));
	zassert_equal(ret, -EAGAIN,
		      "k_condvar_wait(K_FOREVER) did not stay blocked (ret %d)", ret);

	k_thread_abort(&condvar_tid);
}

/**
 * @brief Verify k_condvar_wait() with a zero timeout returns -EAGAIN immediately.
 *
 * @details
 * A helper thread calls k_condvar_wait() with a timeout of 0 ticks. With no
 * signal pending, the call must not block and must return -EAGAIN immediately.
 *
 * Expected result:
 * - k_condvar_wait() returns -EAGAIN (verified in condvar_wait_task()).
 *
 * @see k_condvar_wait()
 */
ZTEST_USER(condvar_tests, test_condvar_wait_nowait)
{
	timeout = 0;

	k_thread_create(&condvar_tid, stack_1, STACK_SIZE,
			condvar_wait_task, &timeout, NULL, NULL,
			PRIO_WAIT, K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	/* giving time for the condvar_wait_task to execute */
	k_sleep(K_MSEC(100));

	k_thread_abort(&condvar_tid);
}


/**
 * @brief Verify a zero-timeout wait returns -EAGAIN even when a signal follows.
 *
 * @details
 * A helper thread calls k_condvar_wait() with a 0 tick timeout and returns
 * -EAGAIN without blocking. Only afterwards does a second thread signal the
 * condition variable. The test confirms the non-blocking wait result is
 * unaffected by the later signal.
 *
 * Expected result:
 * - k_condvar_wait() returns -EAGAIN (verified in condvar_wait_wake_task()).
 * - k_condvar_signal() returns 0 with no waiter present.
 *
 * @see k_condvar_wait()
 * @see k_condvar_signal()
 */
ZTEST_USER(condvar_tests, test_condvar_nowait_returns_eagain)
{
	woken = 0;
	timeout = 0;

	k_thread_create(&condvar_tid, stack_1, STACK_SIZE,
			condvar_wait_wake_task, &timeout, NULL, NULL,
			PRIO_WAIT, K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	/* giving time for the condvar_wait_wake_task to execute */
	k_sleep(K_MSEC(100));

	k_thread_create(&condvar_wake_tid, condvar_wake_stack, STACK_SIZE,
			condvar_wake_task, &woken, NULL, NULL,
			PRIO_WAKE, K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	/* giving time for the condvar_wake_task to execute */
	k_yield();

	k_thread_abort(&condvar_wake_tid);
	k_thread_abort(&condvar_tid);
}


/**
 * @brief Verify a thread blocked forever is woken by a signal from ISR context.
 *
 * @details
 * A helper thread blocks in k_condvar_wait() with K_FOREVER. The test then
 * issues k_condvar_signal() from an ISR via irq_offload(). The test verifies the
 * signal raised in interrupt context releases the waiter, which returns 0.
 *
 * Expected result:
 * - k_condvar_wait() returns 0 (verified in condvar_wait_wake_task()).
 *
 * @note This is a kernel-mode test (ZTEST, not ZTEST_USER) because signalling is
 *       performed from an ISR.
 *
 * @see k_condvar_wait()
 * @see k_condvar_signal()
 */
ZTEST(condvar_tests, test_condvar_wait_forever_wake_from_isr)
{
	timeout = K_TICKS_FOREVER;

	k_thread_create(&condvar_tid, stack_1, STACK_SIZE,
			condvar_wait_wake_task, &timeout, NULL, NULL,
			PRIO_WAIT, K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	/* giving time for the condvar_wait_wake_task to execute */
	k_yield();

	condvar_wake_from_isr(&simple_condvar);

	/* giving time for the condvar_wait_wake_task to execute */
	k_yield();

	k_thread_abort(&condvar_tid);
}

/**
 * @brief Verify k_condvar_broadcast() wakes every thread on one condition variable.
 *
 * @details
 * TOTAL_THREADS_WAITING (3) helper threads all block in k_condvar_wait() with
 * K_FOREVER on the same condition variable. A waker thread then calls
 * k_condvar_broadcast(). The test verifies broadcast reports exactly the number
 * of threads it released and that all waiters return 0.
 *
 * Expected result:
 * - k_condvar_broadcast() returns TOTAL_THREADS_WAITING.
 * - Each k_condvar_wait() returns 0 (verified in condvar_wait_wake_task()).
 *
 * @see k_condvar_broadcast()
 * @see k_condvar_wait()
 */
ZTEST_USER(condvar_tests, test_condvar_multiple_threads_wait_wake)
{
	if (CONFIG_MP_MAX_NUM_CPUS > 1) {
		/*
		 * Asserting an exact broadcast wake count relies on all waiters
		 * having pended before the waker runs, which is not guaranteed
		 * when waiters can start concurrently on other CPUs.
		 */
		ztest_test_skip();
		return;
	}

	timeout = K_TICKS_FOREVER;
	woken = TOTAL_THREADS_WAITING;

	k_condvar_init(&simple_condvar);
	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {

		k_thread_create(&multiple_tid[i], multiple_stack[i],
				STACK_SIZE, condvar_wait_wake_task,
				&timeout, NULL, NULL,
				PRIO_WAIT, K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	}

	/* giving time for the other threads to execute */
	k_yield();

	k_thread_create(&condvar_wake_tid, condvar_wake_stack,
			STACK_SIZE, condvar_wake_multiple, &woken,
			NULL, NULL, PRIO_WAKE,
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	/* giving time for the other threads to execute */
	k_yield();

	k_thread_abort(&condvar_wake_tid);
	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		k_thread_abort(&multiple_tid[i]);
	}
}


void condvar_multiple_wait_wake_task(void *p1, void *p2, void *p3)
{
	int32_t ret_value;
	int time_val = *(int *)p1;
	int idx = POINTER_TO_INT(p2);

	k_condvar_init(&multiple_condvar[idx]);

	zassert_true(time_val == (int)K_TICKS_FOREVER, "invalid timeout parameter");
	k_mutex_lock(&test_mutex, K_FOREVER);

	ret_value = k_condvar_wait(&multiple_condvar[idx],
		&test_mutex, K_TICKS(time_val));
	zassert_true(ret_value == 0, "k_condvar_wait failed.");

	k_mutex_unlock(&test_mutex);
}

void condvar_multiple_wake_task(void *p1, void *p2, void *p3)
{
	int32_t ret_value;
	int woken_num = *(int *)p1;
	int idx = POINTER_TO_INT(p2);

	zassert_true(woken_num > 0, "invalid woken number");

	if (woken > 1) {
		ret_value = k_condvar_signal(&multiple_condvar[idx]);
	} else {
		ret_value = k_condvar_broadcast(&multiple_condvar[idx]);
	}

	zassert_true(ret_value == woken_num, "k_condvar_wake failed. (%d!=%d)",
		     ret_value, woken_num);
}


/**
 * @brief Verify independent condition variables each wake their own single waiter.
 *
 * @details
 * TOTAL_THREADS_WAITING (3) helper threads each block on a distinct condition
 * variable (multiple_condvar[i]). A matching set of waker threads each wakes its
 * own condition variable. With one waiter per condition variable, each wake must
 * release exactly one thread, demonstrating that condition variables are
 * independent and do not cross-signal.
 *
 * Expected result:
 * - Each wake call releases exactly one waiter (returns 1).
 * - Each k_condvar_wait() returns 0 (verified in
 *   condvar_multiple_wait_wake_task()).
 *
 * @note Easily confused with test_condvar_multiple_threads_wait_wake, which has
 *       many waiters on a SINGLE condition variable. This one is the "one
 *       condition variable per waiter" case. Because woken is 1,
 *       condvar_multiple_wake_task() always takes its k_condvar_broadcast()
 *       branch, so its k_condvar_signal() path is not exercised here.
 *
 * @see k_condvar_signal()
 * @see k_condvar_broadcast()
 */
ZTEST_USER(condvar_tests, test_multiple_condvar_wait_wake)
{
	if (CONFIG_MP_MAX_NUM_CPUS > 1) {
		/*
		 * Each waker asserts it woke exactly one thread, which assumes
		 * the matching waiter has already pended. That ordering is not
		 * guaranteed when waiters run concurrently on multiple CPUs.
		 */
		ztest_test_skip();
		return;
	}

	woken = 1;
	timeout = K_TICKS_FOREVER;

	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		k_thread_create(&multiple_tid[i], multiple_stack[i], STACK_SIZE,
				condvar_multiple_wait_wake_task, &timeout, INT_TO_POINTER(i), NULL,
				PRIO_WAIT, K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	}

	/* giving time for the other threads to execute */
	k_msleep(10);

	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		k_thread_create(&multiple_wake_tid[i], multiple_wake_stack[i], STACK_SIZE,
				condvar_multiple_wake_task, &woken, INT_TO_POINTER(i), NULL,
				PRIO_WAKE, K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	}

	/* giving time for the other threads to execute */
	k_yield();

	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		;
	}

	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		k_thread_abort(&multiple_tid[i]);
		k_thread_abort(&multiple_wake_tid[i]);
	}
}

#ifdef CONFIG_USERSPACE
static void cond_init_null(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	ztest_set_fault_valid(true);
	k_condvar_init(NULL);

	/* should not go here*/
	ztest_test_fail();
}


/**
 * @brief Verify k_condvar_init(NULL) faults when called from user mode.
 *
 * @details
 * A user-mode thread arms the ztest fault hook and calls k_condvar_init() with a
 * NULL pointer. The call must trigger a kernel fault; execution must not fall
 * through to the ztest_test_fail() guard placed after it.
 *
 * Expected result:
 * - A valid fault is taken at k_condvar_init(NULL); the guard ztest_test_fail()
 *   in cond_init_null() is never reached.
 *
 * @see k_condvar_init()
 */
ZTEST_USER(condvar_tests, test_condvar_init_null)
{
	k_tid_t tid = k_thread_create(&condvar_tid, stack_1, STACK_SIZE,
			cond_init_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(0),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}
#else
ZTEST_USER(condvar_tests, test_condvar_init_null)
{
	ztest_test_skip();
}
#endif

#ifdef CONFIG_USERSPACE
static void cond_signal_null(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	ztest_set_fault_valid(true);
	k_condvar_signal(NULL);

	/* should not go here*/
	ztest_test_fail();
}

static void cond_broadcast_null(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	ztest_set_fault_valid(true);
	k_condvar_broadcast(NULL);

	/* should not go here*/
	ztest_test_fail();
}

static void cond_wait_null(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	ztest_set_fault_valid(true);
	k_condvar_wait(NULL, NULL, K_FOREVER);

	/* should not go here*/
	ztest_test_fail();
}
/**
 * @brief Verify k_condvar_signal(NULL) faults when called from user mode.
 *
 * @details
 * A user-mode thread arms the ztest fault hook and calls k_condvar_signal() with
 * a NULL pointer. The call must trigger a kernel fault; execution must not fall
 * through to the ztest_test_fail() guard placed after it.
 *
 * Expected result:
 * - A valid fault is taken at k_condvar_signal(NULL); the guard ztest_test_fail()
 *   in cond_signal_null() is never reached.
 *
 * @see k_condvar_signal()
 */
ZTEST_USER(condvar_tests, test_condvar_signal_null)
{
	k_tid_t tid = k_thread_create(&condvar_tid, stack_1, STACK_SIZE,
			cond_signal_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(0),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	k_thread_join(tid, K_FOREVER);
}

/**
 * @brief Verify k_condvar_broadcast(NULL) faults when called from user mode.
 *
 * @details
 * A user-mode thread arms the ztest fault hook and calls k_condvar_broadcast()
 * with a NULL pointer. The call must trigger a kernel fault; execution must not
 * fall through to the ztest_test_fail() guard placed after it.
 *
 * Expected result:
 * - A valid fault is taken at k_condvar_broadcast(NULL); the guard
 *   ztest_test_fail() in cond_broadcast_null() is never reached.
 *
 * @see k_condvar_broadcast()
 */
ZTEST_USER(condvar_tests, test_condvar_broadcast_null)
{
	k_tid_t tid = k_thread_create(&condvar_tid, stack_1, STACK_SIZE,
			cond_broadcast_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(0),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}

/**
 * @brief Verify k_condvar_wait(NULL, NULL, ...) faults when called from user mode.
 *
 * @details
 * A user-mode thread arms the ztest fault hook and calls k_condvar_wait() with
 * NULL condition variable and mutex pointers. The call must trigger a kernel
 * fault; execution must not fall through to the ztest_test_fail() guard placed
 * after it.
 *
 * Expected result:
 * - A valid fault is taken at k_condvar_wait(NULL, NULL, ...); the guard
 *   ztest_test_fail() in cond_wait_null() is never reached.
 *
 * @see k_condvar_wait()
 */
ZTEST_USER(condvar_tests, test_condvar_wait_null)
{
	k_tid_t tid = k_thread_create(&condvar_tid, stack_1, STACK_SIZE,
			cond_wait_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(0),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	k_thread_join(tid, K_FOREVER);
}

#else
ZTEST_USER(condvar_tests, test_condvar_signal_null)
{
	ztest_test_skip();
}
ZTEST_USER(condvar_tests, test_condvar_broadcast_null)
{
	ztest_test_skip();
}
ZTEST_USER(condvar_tests, test_condvar_wait_null)
{
	ztest_test_skip();
}
#endif


void inc_count(void *p1, void *p2, void *p3)
{
	int i;
	long multi = (long)p2;

	for (i = 0; i < TCOUNT; i++) {
		k_mutex_lock(&test_mutex, K_FOREVER);
		count++;

		if (count == COUNT_LIMIT) {
			if (multi) {
				k_condvar_broadcast(&simple_condvar);
			} else {
				k_condvar_signal(&simple_condvar);
			}
		}

		k_mutex_unlock(&test_mutex);

		/* Sleep so threads can alternate on mutex lock */
		k_sleep(K_MSEC(50));
	}
}

void watch_count(void *p1, void *p2, void *p3)
{
	long my_id = (long)p1;

	printk("Starting %s: thread %ld\n", __func__, my_id);

	k_mutex_lock(&test_mutex, K_FOREVER);
	while (count < COUNT_LIMIT) {
		k_condvar_wait(&simple_condvar, &test_mutex, K_FOREVER);
	}
	count += 125;
	k_mutex_unlock(&test_mutex);
}

void _condvar_usecase(long multi)
{
	long t1 = 1, t2 = 2, t3 = 3;
	int i;

	count = 0;

	/* Reinit mutex to prevent affection from previous testcases */
	k_mutex_init(&test_mutex);

	k_thread_create(&multiple_tid[0], multiple_stack[0], STACK_SIZE, watch_count,
			INT_TO_POINTER(t1), NULL, NULL, K_PRIO_PREEMPT(10),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_create(&multiple_tid[1], multiple_stack[1], STACK_SIZE, inc_count,
			INT_TO_POINTER(t2), INT_TO_POINTER(multi), NULL, K_PRIO_PREEMPT(10),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_create(&multiple_tid[2], multiple_stack[2], STACK_SIZE, inc_count,
			INT_TO_POINTER(t3), INT_TO_POINTER(multi), NULL, K_PRIO_PREEMPT(10),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	/* Wait for all threads to complete */
	for (i = 0; i < 3; i++) {
		k_thread_join(&multiple_tid[i], K_FOREVER);
	}

	zassert_equal(count, 145, "Count not equal to 145");

}


/**
 * @brief Verify a producer/consumer hand-off driven by k_condvar_signal().
 *
 * @details
 * One watcher thread holds the mutex and waits on the condition variable until a
 * shared counter reaches COUNT_LIMIT; two incrementer threads raise the counter
 * under the same mutex and call k_condvar_signal() when the limit is hit. The
 * deterministic final counter value proves the single-waiter wakeup fired and
 * the mutex-protected shared state stayed consistent.
 *
 * Expected result:
 * - Final count equals 145: 20 increments from the two incrementer threads
 *   (2 x TCOUNT) plus 125 added by the watcher after it is released
 *   (verified by zassert_equal() in _condvar_usecase()).
 *
 * @see k_condvar_signal()
 * @see k_condvar_wait()
 */
ZTEST_USER(condvar_tests, test_condvar_usecase_signal)
{
	_condvar_usecase(0);
}


/**
 * @brief Verify a producer/consumer hand-off driven by k_condvar_broadcast().
 *
 * @details
 * Same producer/consumer scenario as test_condvar_usecase_signal, but the
 * incrementer threads release the watcher with k_condvar_broadcast() instead of
 * k_condvar_signal(). With a single watcher the observable outcome is identical;
 * this exercises the broadcast wakeup path in the same use case.
 *
 * Expected result:
 * - Final count equals 145 (verified by zassert_equal() in _condvar_usecase()).
 *
 * @see k_condvar_broadcast()
 * @see k_condvar_wait()
 */
ZTEST_USER(condvar_tests, test_condvar_usecase_broadcast)
{
	_condvar_usecase(1);
}

/*test case main entry*/
static void *condvar_tests_setup(void)
{
#ifdef CONFIG_USERSPACE
	k_thread_access_grant(k_current_get(), &test_mutex, &condvar_tid, &condvar_wake_tid,
				&simple_condvar, &stack_1, &condvar_wake_stack);

	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		k_thread_access_grant(k_current_get(),
				      &multiple_tid[i],
				      &multiple_wake_tid[i],
				      &multiple_stack[i],
				      &multiple_condvar[i],
				      &multiple_wake_stack[i]);
	}
#endif
	/*
	 * Initialize the shared condition variable here so that no individual
	 * test depends on a prior test having initialized it. Several tests use
	 * helpers (e.g. condvar_wait_wake_task()) that wait on simple_condvar
	 * without calling k_condvar_init() themselves.
	 */
	k_condvar_init(&simple_condvar);

	return NULL;
}

/**
 * @brief Verify k_condvar_wait() re-locks the mutex when the wait times out.
 *
 * @details
 * Per the contract documented in kernel/condvar.c, k_condvar_wait() must always
 * return with the mutex held by the calling thread, even when the wait times out
 * (matching POSIX semantics for pthread_cond_timedwait). The test locks a mutex,
 * waits with a short timeout and no waker so the wait expires, then unlocks the
 * mutex to prove ownership was restored.
 *
 * Expected result:
 * - k_condvar_wait() returns -EAGAIN on timeout.
 * - k_mutex_unlock() succeeds; a regression that failed to re-lock the mutex
 *   would make it return -EPERM (mutex not locked by the calling thread).
 *
 * @see k_condvar_wait()
 * @see k_mutex_unlock()
 */
ZTEST(condvar_tests, test_condvar_wait_timeout_relocks_mutex)
{
	/*
	 * Keep these off the thread stack: on CONFIG_KERNEL_COHERENCE
	 * platforms (e.g. intel_adsp) stacks are cached/non-coherent, but a
	 * pended-on wait_q must be memory-coherent (see the assert in
	 * pend_locked() at kernel/sched.c). static places them in coherent
	 * .bss, matching every other kernel object in this file.
	 */
	static struct k_condvar local_cv;
	static struct k_mutex local_mtx;
	int ret;

	zassert_ok(k_condvar_init(&local_cv));
	zassert_ok(k_mutex_init(&local_mtx));

	/* Acquire the mutex before waiting. */
	zassert_ok(k_mutex_lock(&local_mtx, K_FOREVER));

	/* No waker, so this must time out. */
	ret = k_condvar_wait(&local_cv, &local_mtx, K_MSEC(10));
	zassert_equal(ret, -EAGAIN,
		      "expected -EAGAIN on timeout, got %d", ret);

	/*
	 * The kernel contract requires the mutex to be locked on return.
	 * k_mutex_unlock() returns -EPERM if the mutex is not locked
	 * by the calling thread — that would mean the fix regressed.
	 */
	zassert_ok(k_mutex_unlock(&local_mtx),
		   "mutex not held after k_condvar_wait timeout");
}

/* Helper: block forever on simple_condvar, then record the wakeup. */
static void condvar_count_waiter(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_mutex_lock(&test_mutex, K_FOREVER);
	(void)k_condvar_wait(&simple_condvar, &test_mutex, K_FOREVER);
	woken_count++;
	k_mutex_unlock(&test_mutex);
}

/* Helper: block forever on simple_condvar, then append this id to wake_order. */
static void condvar_prio_waiter(void *p1, void *p2, void *p3)
{
	int id = POINTER_TO_INT(p1);

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_mutex_lock(&test_mutex, K_FOREVER);
	(void)k_condvar_wait(&simple_condvar, &test_mutex, K_FOREVER);
	wake_order[wake_idx] = id;
	wake_idx++;
	k_mutex_unlock(&test_mutex);
}

/* Helper: sleep p1 milliseconds, then signal the condvar passed in p2. */
static void condvar_signal_after(void *p1, void *p2, void *p3)
{
	int delay_ms = POINTER_TO_INT(p1);
	struct k_condvar *condvar = p2;

	ARG_UNUSED(p3);

	k_msleep(delay_ms);
	k_condvar_signal(condvar);
}

/* Helper: probe recursive_mtx with a short timeout while the main thread waits. */
static void recursive_mtx_contender(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	contender_ret = k_mutex_lock(&recursive_mtx, K_MSEC(50));
	if (contender_ret == 0) {
		k_mutex_unlock(&recursive_mtx);
	}
}

/**
 * @brief Verify k_condvar_signal() releases exactly one of several waiters.
 *
 * @details
 * TOTAL_THREADS_WAITING (3) helper threads all block on the same condition
 * variable with K_FOREVER. A single k_condvar_signal() must release exactly one
 * of them; the others must stay blocked. This is the defining difference between
 * signal and broadcast and was previously only covered for broadcast.
 *
 * Expected result:
 * - k_condvar_signal() returns 0.
 * - Exactly one waiter runs past k_condvar_wait() (woken_count == 1); the rest
 *   remain blocked until a later broadcast drains them.
 *
 * @see k_condvar_signal()
 * @see k_condvar_broadcast()
 */
ZTEST_USER(condvar_tests, test_condvar_signal_wakes_one)
{
	woken_count = 0;
	k_condvar_init(&simple_condvar);
	k_mutex_init(&test_mutex);

	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		k_thread_create(&multiple_tid[i], multiple_stack[i], STACK_SIZE,
				condvar_count_waiter, NULL, NULL, NULL,
				PRIO_WAIT, K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	}

	/* Let every waiter reach k_condvar_wait(). */
	k_msleep(50);

	zassert_equal(k_condvar_signal(&simple_condvar), 0, "signal failed");

	/* Give the single woken waiter time to run. */
	k_msleep(50);
	zassert_equal(woken_count, 1,
		      "signal woke %d threads, expected exactly 1", woken_count);

	/* Release and reap the remaining waiters. */
	k_condvar_broadcast(&simple_condvar);
	k_msleep(50);
	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		k_thread_abort(&multiple_tid[i]);
	}
}

/**
 * @brief Verify k_condvar_signal() wakes the highest-priority waiter first.
 *
 * @details
 * Three helper threads block on the same condition variable at distinct, lower
 * priorities than the test thread. The test signals once per iteration and each
 * woken thread appends its id to wake_order[]. The wakeups must follow priority
 * order (highest priority first), not creation/FIFO order.
 *
 * Expected result:
 * - wake_order[] is the waiter ids in descending-priority order.
 *
 * @note Skipped when CONFIG_MP_MAX_NUM_CPUS > 1: with multiple CPUs waiters can
 *       run concurrently, so a strict wakeup order is not guaranteed.
 *
 * @see k_condvar_signal()
 */
ZTEST_USER(condvar_tests, test_condvar_signal_wakes_highest_priority)
{
	int base;

	if (CONFIG_MP_MAX_NUM_CPUS > 1) {
		ztest_test_skip();
		return;
	}

	wake_idx = 0;
	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		wake_order[i] = -1;
	}
	k_condvar_init(&simple_condvar);
	k_mutex_init(&test_mutex);

	base = k_thread_priority_get(k_current_get());

	/*
	 * Waiter i gets priority base + (TOTAL_THREADS_WAITING - i), so the
	 * last waiter created (largest id) is the highest priority. All are
	 * lower priority than this thread, so signalling never preempts us.
	 */
	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		k_thread_create(&multiple_tid[i], multiple_stack[i], STACK_SIZE,
				condvar_prio_waiter, INT_TO_POINTER(i), NULL, NULL,
				base + (TOTAL_THREADS_WAITING - i),
				K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	}

	/* Let every waiter reach k_condvar_wait(). */
	k_msleep(50);

	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		zassert_equal(k_condvar_signal(&simple_condvar), 0, "signal failed");
		k_msleep(50);
	}

	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		int expected = TOTAL_THREADS_WAITING - 1 - i;

		zassert_equal(wake_order[i], expected,
			      "wake slot %d was waiter %d, expected %d",
			      i, wake_order[i], expected);
	}

	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		k_thread_abort(&multiple_tid[i]);
	}
}

/**
 * @brief Verify the K_NO_WAIT path of k_condvar_wait() leaves the mutex held.
 *
 * @details
 * k_condvar_wait() with K_NO_WAIT returns -EAGAIN without touching the mutex
 * (a distinct code path from the timeout path). This test locks a mutex, makes
 * the no-wait call, then unlocks to prove the caller still owns it.
 *
 * Expected result:
 * - k_condvar_wait() returns -EAGAIN.
 * - k_mutex_unlock() succeeds (returns -EPERM if ownership was lost).
 *
 * @see k_condvar_wait()
 * @see k_mutex_unlock()
 */
ZTEST(condvar_tests, test_condvar_wait_nowait_keeps_mutex)
{
	struct k_condvar cv;
	struct k_mutex mtx;

	zassert_ok(k_condvar_init(&cv));
	zassert_ok(k_mutex_init(&mtx));
	zassert_ok(k_mutex_lock(&mtx, K_FOREVER));

	zassert_equal(k_condvar_wait(&cv, &mtx, K_NO_WAIT), -EAGAIN,
		      "K_NO_WAIT wait must return -EAGAIN");

	zassert_ok(k_mutex_unlock(&mtx),
		   "mutex not held after K_NO_WAIT k_condvar_wait()");
}

/**
 * @brief Verify a signaled k_condvar_wait() returns with the mutex held.
 *
 * @details
 * Complements the timeout-relock test for the success path: a waker thread
 * signals the condition variable while the test thread is blocked in
 * k_condvar_wait(). On return the mutex must be re-acquired by the caller.
 *
 * Expected result:
 * - k_condvar_wait() returns 0 (signaled).
 * - k_mutex_unlock() succeeds (returns -EPERM if the mutex was not re-locked).
 *
 * @see k_condvar_wait()
 * @see k_mutex_unlock()
 */
ZTEST(condvar_tests, test_condvar_wait_signaled_keeps_mutex)
{
	struct k_mutex mtx;
	int ret;

	zassert_ok(k_condvar_init(&simple_condvar));
	zassert_ok(k_mutex_init(&mtx));

	k_thread_create(&condvar_wake_tid, condvar_wake_stack, STACK_SIZE,
			condvar_signal_after, INT_TO_POINTER(50), &simple_condvar,
			NULL, PRIO_WAKE, 0, K_NO_WAIT);

	zassert_ok(k_mutex_lock(&mtx, K_FOREVER));
	ret = k_condvar_wait(&simple_condvar, &mtx, K_FOREVER);
	zassert_equal(ret, 0, "expected signaled wakeup, got %d", ret);

	zassert_ok(k_mutex_unlock(&mtx),
		   "mutex not held after signaled k_condvar_wait()");

	k_thread_join(&condvar_wake_tid, K_FOREVER);
}

/**
 * @brief Verify a statically defined condition variable works without init.
 *
 * @details
 * static_condvar is created with K_CONDVAR_DEFINE() and never passed to
 * k_condvar_init(). The test exercises signal/broadcast with no waiters and a
 * full wait/signal round-trip to confirm the static initializer produces a
 * usable object.
 *
 * Expected result:
 * - k_condvar_signal()/k_condvar_broadcast() with no waiter return 0.
 * - A blocked k_condvar_wait() is released by a signal and returns 0.
 *
 * @see K_CONDVAR_DEFINE
 * @see k_condvar_wait()
 */
ZTEST(condvar_tests, test_condvar_static_define)
{
	struct k_mutex mtx;
	int ret;

	zassert_ok(k_mutex_init(&mtx));

	/* No k_condvar_init(&static_condvar): it is initialized at build time. */
	zassert_equal(k_condvar_signal(&static_condvar), 0,
		      "signal on static condvar with no waiter must return 0");
	zassert_equal(k_condvar_broadcast(&static_condvar), 0,
		      "broadcast on static condvar with no waiter must return 0");

	k_thread_create(&condvar_wake_tid, condvar_wake_stack, STACK_SIZE,
			condvar_signal_after, INT_TO_POINTER(50), &static_condvar,
			NULL, PRIO_WAKE, 0, K_NO_WAIT);

	zassert_ok(k_mutex_lock(&mtx, K_FOREVER));
	ret = k_condvar_wait(&static_condvar, &mtx, K_FOREVER);
	zassert_equal(ret, 0, "static condvar wait not signaled (ret %d)", ret);
	zassert_ok(k_mutex_unlock(&mtx));

	k_thread_join(&condvar_wake_tid, K_FOREVER);
}

/**
 * @brief Verify k_condvar_signal()/k_condvar_broadcast() with no waiters return 0.
 *
 * @details
 * Exercises the no-waiter branches: with an empty wait queue, signal and
 * broadcast must be no-ops that report zero woken threads.
 *
 * Expected result:
 * - k_condvar_signal() returns 0.
 * - k_condvar_broadcast() returns 0.
 *
 * @see k_condvar_signal()
 * @see k_condvar_broadcast()
 */
ZTEST_USER(condvar_tests, test_condvar_wake_no_waiters)
{
	k_condvar_init(&simple_condvar);

	zassert_equal(k_condvar_signal(&simple_condvar), 0,
		      "signal with no waiters must return 0");
	zassert_equal(k_condvar_broadcast(&simple_condvar), 0,
		      "broadcast with no waiters must return 0");
}

/**
 * @brief Document that k_condvar_wait() does not fully release a recursive mutex.
 *
 * @details
 * Unlike POSIX pthread_cond_wait(), which releases a recursively locked mutex
 * completely and restores the count on return, k_condvar_wait() calls
 * k_mutex_unlock() exactly once. When the mutex is locked recursively the count
 * is only decremented, so the waiting thread keeps ownership across the wait.
 *
 * The test locks a mutex twice, then waits on a condition variable. While it is
 * blocked, a contender thread tries to acquire the mutex with a short timeout
 * and must fail, proving the mutex was not released. A separate thread signals
 * the wait afterwards so the test can unwind cleanly.
 *
 * Expected result:
 * - The contender's k_mutex_lock() times out with -EAGAIN (current behaviour).
 * - k_condvar_wait() returns 0 once signaled and both recursive levels unlock.
 *
 * @note KNOWN LIMITATION / divergence from POSIX. If kernel/condvar.c is changed
 *       to fully release a recursively locked mutex, this expectation must be
 *       updated (the contender would then succeed).
 *
 * @see k_condvar_wait()
 * @see k_mutex_lock()
 */
ZTEST(condvar_tests, test_condvar_wait_recursive_mutex_not_released)
{
	int base = k_thread_priority_get(k_current_get());
	int ret;

	contender_ret = 0;
	zassert_ok(k_condvar_init(&simple_condvar));
	zassert_ok(k_mutex_init(&recursive_mtx));

	/* Lock the mutex twice (recursive ownership, lock_count == 2). */
	zassert_ok(k_mutex_lock(&recursive_mtx, K_FOREVER));
	zassert_ok(k_mutex_lock(&recursive_mtx, K_FOREVER));

	/*
	 * Helpers run at lower priority so they only execute once this thread
	 * parks in k_condvar_wait(). The contender probes the mutex during the
	 * wait window (50 ms timeout); the signaller wakes us only afterwards
	 * (150 ms) so the probe deterministically times out first.
	 */
	k_thread_create(&multiple_tid[0], multiple_stack[0], STACK_SIZE,
			recursive_mtx_contender, NULL, NULL, NULL,
			base + 1, 0, K_NO_WAIT);
	k_thread_create(&condvar_wake_tid, condvar_wake_stack, STACK_SIZE,
			condvar_signal_after, INT_TO_POINTER(150), &simple_condvar,
			NULL, base + 2, 0, K_NO_WAIT);

	ret = k_condvar_wait(&simple_condvar, &recursive_mtx, K_FOREVER);
	zassert_equal(ret, 0, "expected signaled wakeup, got %d", ret);

	k_thread_join(&multiple_tid[0], K_FOREVER);

	zassert_equal(contender_ret, -EAGAIN,
		      "recursive mutex released during wait (ret %d) -- POSIX-correct "
		      "now? update this test", contender_ret);

	/* This thread still owns the recursive lock; unwind both levels. */
	zassert_ok(k_mutex_unlock(&recursive_mtx));
	zassert_ok(k_mutex_unlock(&recursive_mtx));

	k_thread_join(&condvar_wake_tid, K_FOREVER);
}

/**
 * @}
 */

ZTEST_SUITE(condvar_tests, NULL, condvar_tests_setup, NULL, NULL, NULL);
