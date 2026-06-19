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
	struct k_condvar local_cv;
	struct k_mutex local_mtx;
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

/**
 * @}
 */

ZTEST_SUITE(condvar_tests, NULL, condvar_tests_setup, NULL, NULL, NULL);
