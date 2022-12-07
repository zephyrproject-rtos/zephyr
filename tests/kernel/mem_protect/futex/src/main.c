/*
 * Copyright (c) 2019, 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/irq_offload.h>
#include <zephyr/sys/mutex.h>


/**
 * @brief Tests for Kernel Futex objects
 * @defgroup kernel_futex_tests Futex
 * @ingroup all_tests
 * @{
 * @}
 */

/* Macro declarations */
#define TOTAL_THREADS_WAITING (3)
#define PRIO_WAIT (CONFIG_ZTEST_THREAD_PRIORITY - 1)
#define PRIO_WAKE (CONFIG_ZTEST_THREAD_PRIORITY - 2)
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define PRIORITY 5

/******************************************************************************/
/* declaration */
K_THREAD_STACK_DEFINE(stack_1, STACK_SIZE);
K_THREAD_STACK_DEFINE(futex_wake_stack, STACK_SIZE);
K_THREAD_STACK_ARRAY_DEFINE(multiple_stack,
		TOTAL_THREADS_WAITING, STACK_SIZE);
K_THREAD_STACK_ARRAY_DEFINE(multiple_wake_stack,
		TOTAL_THREADS_WAITING, STACK_SIZE);

ZTEST_BMEM int woken;
ZTEST_BMEM int timeout;
ZTEST_BMEM int index[TOTAL_THREADS_WAITING];
ZTEST_BMEM struct k_futex simple_futex;
ZTEST_BMEM struct k_futex multiple_futex[TOTAL_THREADS_WAITING];
struct k_futex no_access_futex;
ZTEST_BMEM atomic_t not_a_futex;
ZTEST_BMEM struct sys_mutex also_not_a_futex;

struct k_thread futex_tid;
struct k_thread futex_wake_tid;
struct k_thread multiple_tid[TOTAL_THREADS_WAITING];
struct k_thread multiple_wake_tid[TOTAL_THREADS_WAITING];

/******************************************************************************/
/* Helper functions */
static void futex_isr_wake(const void *futex)
{
	k_futex_wake((struct k_futex *)futex, false);
}

static void futex_wake_from_isr(struct k_futex *futex)
{
	irq_offload(futex_isr_wake, (const void *)futex);
}

/* test futex wait, no futex wake */
static void futex_wait_task(void *p1, void *p2, void *p3)
{
	int32_t ret_value;
	k_ticks_t time_val = *(int *)p1;

	zassert_true(time_val >= (int)K_TICKS_FOREVER,
		     "invalid timeout parameter");

	ret_value = k_futex_wait(&simple_futex,
			atomic_get(&simple_futex.val), K_TICKS(time_val));

	switch (time_val) {
	case K_TICKS_FOREVER:
		zassert_true(ret_value == 0,
		     "k_futex_wait failed when it shouldn't have");
		zassert_false(ret_value == 0,
		     "futex wait task wakeup when it shouldn't have");
		break;
	case 0:
		zassert_true(ret_value == -ETIMEDOUT,
		     "k_futex_wait failed when it shouldn't have");
		atomic_sub(&simple_futex.val, 1);
		break;
	default:
		zassert_true(ret_value == -ETIMEDOUT,
		     "k_futex_wait failed when it shouldn't have");
		atomic_sub(&simple_futex.val, 1);
		break;
	}
}

static void futex_wake_task(void *p1, void *p2, void *p3)
{
	int32_t ret_value;
	int woken_num = *(int *)p1;

	ret_value = k_futex_wake(&simple_futex,
				woken_num == 1 ? false : true);
	zassert_true(ret_value == woken_num,
		"k_futex_wake failed when it shouldn't have");
}

static void futex_wait_wake_task(void *p1, void *p2, void *p3)
{
	int32_t ret_value;
	int time_val = *(int *)p1;

	zassert_true(time_val >= (int)K_TICKS_FOREVER, "invalid timeout parameter");

	ret_value = k_futex_wait(&simple_futex,
			atomic_get(&simple_futex.val), K_TICKS(time_val));

	switch (time_val) {
	case K_TICKS_FOREVER:
		zassert_true(ret_value == 0,
		     "k_futex_wait failed when it shouldn't have");
		break;
	case 0:
		zassert_true(ret_value == -ETIMEDOUT,
		     "k_futex_wait failed when it shouldn't have");
		break;
	default:
		zassert_true(ret_value == 0,
		     "k_futex_wait failed when it shouldn't have");
		break;
	}

	atomic_sub(&simple_futex.val, 1);
}

static void futex_multiple_wake_task(void *p1, void *p2, void *p3)
{
	int32_t ret_value;
	int woken_num = *(int *)p1;
	int idx = *(int *)p2;

	zassert_true(woken_num > 0, "invalid woken number");

	ret_value = k_futex_wake(&multiple_futex[idx],
			woken_num == 1 ? false : true);
	zassert_true(ret_value == woken_num,
		"k_futex_wake failed when it shouldn't have");
}

static void futex_multiple_wait_wake_task(void *p1, void *p2, void *p3)
{
	int32_t ret_value;
	int time_val = *(int *)p1;
	int idx = *(int *)p2;

	zassert_true(time_val == (int)K_TICKS_FOREVER, "invalid timeout parameter");

	ret_value = k_futex_wait(&multiple_futex[idx],
		atomic_get(&(multiple_futex[idx].val)), K_TICKS(time_val));
	zassert_true(ret_value == 0,
	     "k_futex_wait failed when it shouldn't have");

	atomic_sub(&(multiple_futex[idx].val), 1);
}

/**
 * @ingroup kernel_futex_tests
 * @{
 */

/**
 * @brief Test k_futex_wait() forever
 */
ZTEST(futex, test_futex_wait_forever)
{
	timeout = K_TICKS_FOREVER;

	atomic_set(&simple_futex.val, 1);

	k_thread_create(&futex_tid, stack_1, STACK_SIZE,
			futex_wait_task, &timeout, NULL, NULL,
			PRIO_WAIT, K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	/* giving time for the futex_wait_task to execute */
	k_yield();

	zassert_false(atomic_get(&simple_futex.val) == 0,
			"wait forever shouldn't wake");

	k_thread_abort(&futex_tid);
}

ZTEST(futex, test_futex_wait_timeout)
{
	timeout = k_ms_to_ticks_ceil32(50);

	atomic_set(&simple_futex.val, 1);

	k_thread_create(&futex_tid, stack_1, STACK_SIZE,
			futex_wait_task, &timeout, NULL, NULL,
			PRIO_WAIT, K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	/* giving time for the futex_wait_task to execute */
	k_sleep(K_MSEC(100));

	zassert_true(atomic_get(&simple_futex.val) == 0,
			"wait timeout doesn't timeout");

	k_thread_abort(&futex_tid);
}

ZTEST(futex, test_futex_wait_nowait)
{
	timeout = 0;

	atomic_set(&simple_futex.val, 1);

	k_thread_create(&futex_tid, stack_1, STACK_SIZE,
			futex_wait_task, &timeout, NULL, NULL,
			PRIO_WAIT, K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	/* giving time for the futex_wait_task to execute */
	k_sleep(K_MSEC(100));

	zassert_true(atomic_get(&simple_futex.val) == 0, "wait nowait fail");

	k_thread_abort(&futex_tid);
}

/**
 * @brief Test k_futex_wait() and k_futex_wake()
 */
ZTEST(futex, test_futex_wait_forever_wake)
{
	woken = 1;
	timeout = K_TICKS_FOREVER;

	atomic_set(&simple_futex.val, 1);

	k_thread_create(&futex_tid, stack_1, STACK_SIZE,
			futex_wait_wake_task, &timeout, NULL, NULL,
			PRIO_WAIT, K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	/* giving time for the futex_wait_wake_task to execute */
	k_yield();

	k_thread_create(&futex_wake_tid, futex_wake_stack, STACK_SIZE,
			futex_wake_task, &woken, NULL, NULL,
			PRIO_WAKE, K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	/* giving time for the futex_wake_task
	 * and futex_wait_wake_task to execute
	 */
	k_yield();

	zassert_true(atomic_get(&simple_futex.val) == 0,
			"wait forever doesn't wake");

	k_thread_abort(&futex_wake_tid);
	k_thread_abort(&futex_tid);
}

ZTEST(futex, test_futex_wait_timeout_wake)
{
	woken = 1;
	timeout = k_ms_to_ticks_ceil32(100);

	atomic_set(&simple_futex.val, 1);

	k_thread_create(&futex_tid, stack_1, STACK_SIZE,
			futex_wait_wake_task, &timeout, NULL, NULL,
			PRIO_WAIT, K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	/* giving time for the futex_wait_wake_task to execute */
	k_yield();

	k_thread_create(&futex_wake_tid, futex_wake_stack, STACK_SIZE,
			futex_wake_task, &woken, NULL, NULL,
			PRIO_WAKE, K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	/*
	 * giving time for the futex_wake_task
	 * and futex_wait_wake_task to execute
	 */
	k_yield();

	zassert_true(atomic_get(&simple_futex.val) == 0,
			"wait timeout doesn't wake");

	k_thread_abort(&futex_wake_tid);
	k_thread_abort(&futex_tid);
}

ZTEST(futex, test_futex_wait_nowait_wake)
{
	woken = 0;
	timeout = 0;

	atomic_set(&simple_futex.val, 1);

	k_thread_create(&futex_tid, stack_1, STACK_SIZE,
			futex_wait_wake_task, &timeout, NULL, NULL,
			PRIO_WAIT, K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	/* giving time for the futex_wait_wake_task to execute */
	k_sleep(K_MSEC(100));

	k_thread_create(&futex_wake_tid, futex_wake_stack, STACK_SIZE,
			futex_wake_task, &woken, NULL, NULL,
			PRIO_WAKE, K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	/* giving time for the futex_wake_task to execute */
	k_yield();

	k_thread_abort(&futex_wake_tid);
	k_thread_abort(&futex_tid);
}

ZTEST(futex, test_futex_wait_forever_wake_from_isr)
{
	timeout = K_TICKS_FOREVER;

	atomic_set(&simple_futex.val, 1);

	k_thread_create(&futex_tid, stack_1, STACK_SIZE,
			futex_wait_wake_task, &timeout, NULL, NULL,
			PRIO_WAIT, K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	/* giving time for the futex_wait_wake_task to execute */
	k_yield();

	futex_wake_from_isr(&simple_futex);

	/* giving time for the futex_wait_wake_task to execute */
	k_yield();

	zassert_true(atomic_get(&simple_futex.val) == 0,
			"wait forever wake from isr doesn't wake");

	k_thread_abort(&futex_tid);
}

ZTEST(futex, test_futex_multiple_threads_wait_wake)
{
	timeout = K_TICKS_FOREVER;
	woken = TOTAL_THREADS_WAITING;

	atomic_clear(&simple_futex.val);

	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		atomic_inc(&simple_futex.val);
		k_thread_create(&multiple_tid[i], multiple_stack[i],
				STACK_SIZE, futex_wait_wake_task,
				&timeout, NULL, NULL, PRIO_WAIT,
				K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	}

	/* giving time for the other threads to execute */
	k_yield();

	k_thread_create(&futex_wake_tid, futex_wake_stack,
			STACK_SIZE, futex_wake_task, &woken,
			NULL, NULL, PRIO_WAKE,
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	/* giving time for the other threads to execute */
	k_yield();

	zassert_true(atomic_get(&simple_futex.val) == 0,
			"wait forever wake doesn't wake all threads");

	k_thread_abort(&futex_wake_tid);
	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		k_thread_abort(&multiple_tid[i]);
	}
}

ZTEST(futex, test_multiple_futex_wait_wake)
{
	woken = 1;
	timeout = K_TICKS_FOREVER;

	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		index[i] = i;
		atomic_set(&multiple_futex[i].val, 1);
		k_thread_create(&multiple_tid[i], multiple_stack[i],
				STACK_SIZE, futex_multiple_wait_wake_task,
				&timeout, &index[i], NULL, PRIO_WAIT,
				K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	}

	/* giving time for the other threads to execute */
	k_yield();

	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		k_thread_create(&multiple_wake_tid[i], multiple_wake_stack[i],
				STACK_SIZE, futex_multiple_wake_task,
				&woken, &index[i], NULL, PRIO_WAKE,
				K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	}

	/* giving time for the other threads to execute */
	k_yield();

	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		zassert_true(atomic_get(&multiple_futex[i].val) == 0,
			"wait forever wake doesn't wake %d thread", i);
	}

	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		k_thread_abort(&multiple_tid[i]);
		k_thread_abort(&multiple_wake_tid[i]);
	}
}

ZTEST_USER(futex, test_user_futex_bad)
{
	int ret;

	/* Is a futex, but no access to its memory */
	ret = k_futex_wait(&no_access_futex, 0, K_NO_WAIT);
	zassert_equal(ret, -EACCES, "shouldn't have been able to access");
	ret = k_futex_wake(&no_access_futex, false);
	zassert_equal(ret, -EACCES, "shouldn't have been able to access");

	/* Access to memory, but not a kernel object */
	ret = k_futex_wait((struct k_futex *)&not_a_futex, 0, K_NO_WAIT);
	zassert_equal(ret, -EINVAL, "waited on non-futex");
	ret = k_futex_wake((struct k_futex *)&not_a_futex, false);
	zassert_equal(ret, -EINVAL, "woke non-futex");

	/* Access to memory, but wrong object type */
	ret = k_futex_wait((struct k_futex *)&also_not_a_futex, 0, K_NO_WAIT);
	zassert_equal(ret, -EINVAL, "waited on non-futex");
	ret = k_futex_wake((struct k_futex *)&also_not_a_futex, false);
	zassert_equal(ret, -EINVAL, "woke non-futex");

	/* Wait with unexpected value */
	atomic_set(&simple_futex.val, 100);
	ret = k_futex_wait(&simple_futex, 0, K_NO_WAIT);
	zassert_equal(ret, -EAGAIN, "waited when values did not match");

	/* Timeout case */
	ret = k_futex_wait(&simple_futex, 100, K_NO_WAIT);
	zassert_equal(ret, -ETIMEDOUT, "didn't time out");
}

static void futex_wait_wake(void *p1, void *p2, void *p3)
{
	int32_t ret_value;

	/* Test user thread can make wait without error
	 * Use assertion to verify k_futex_wait() returns 0
	 */
	ret_value = k_futex_wait(&simple_futex, 13, K_FOREVER);
	zassert_equal(ret_value, 0);

	/* Test user thread can make wake without error
	 * Use assertion to verify k_futex_wake() returns 1,
	 * because only 1 thread wakes
	 */
	ret_value = k_futex_wake(&simple_futex, false);
	zassert_equal(ret_value, 1);
}

static void futex_wake(void *p1, void *p2, void *p3)
{
	int32_t atomic_ret_val;
	int32_t ret_value;

	k_futex_wake(&simple_futex, false);

	ret_value = k_futex_wait(&simple_futex, 13, K_FOREVER);
	zassert_equal(ret_value, 0);

	/* Test user can write to the futex value
	 * Use assertion to verify subtraction correctness
	 * Initial value was 13, after atomic_sub() must be 12
	 */
	atomic_sub(&simple_futex.val, 1);
	atomic_ret_val = atomic_get(&simple_futex.val);
	zassert_equal(atomic_ret_val, 12);
}

/**
 * @brief Test kernel supports locating kernel objects without private kernel
 * data anywhere in memory, control access with the memory domain configuration
 *
 * @details For that test kernel object which doesn't contain private kernel
 * data will be used futex. Test performs handshaking between two user threads
 * to test next requirements:
 * - Place a futex simple_futex in user memory using ZTEST_BMEM
 * - Show that user threads can write to futex value
 * - Show that user threads can make wait/wake syscalls on it.
 *
 * @see atomic_set(), atomic_sub(), k_futex_wake(), k_futex_wait()
 *
 * @ingroup kernel_futex_tests
 */
ZTEST_USER(futex, test_futex_locate_access)
{

	atomic_set(&simple_futex.val, 13);

	k_thread_create(&futex_tid, stack_1, STACK_SIZE,
			futex_wait_wake, NULL, NULL, NULL,
			PRIORITY, K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	/* giving time for the futex_wait_wake_task to execute */
	k_yield();

	k_thread_create(&futex_wake_tid, futex_wake_stack, STACK_SIZE,
			futex_wake, NULL, NULL, NULL,
			PRIORITY, K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	/*
	 * giving time for the futex_wake_task
	 * and futex_wait_wake_task to execute
	 */
	k_yield();

	k_thread_abort(&futex_tid);
	k_thread_abort(&futex_wake_tid);
}

/* ztest main entry*/
void *futex_setup(void)
{
	k_thread_access_grant(k_current_get(),
					&futex_tid, &stack_1, &futex_wake_tid, &futex_wake_stack,
					&simple_futex);
	return NULL;
}

ZTEST_SUITE(futex, NULL, futex_setup, NULL, NULL, NULL);
/**
 * @}
 */
