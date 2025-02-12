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
ZTEST_BMEM int index[TOTAL_THREADS_WAITING];
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
		zassert_true(ret_value == 0,
		     "k_condvar_wait failed.");
		zassert_false(ret_value == 0,
		     "condvar wait task wakeup.");
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
 * @brief Test k_condvar_wait() and k_condvar_wake()
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


ZTEST_USER(condvar_tests, test_condvar_wait_timeout_wake)
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
 * @brief Test k_condvar_wait() forever
 */
ZTEST_USER(condvar_tests, test_condvar_wait_forever)
{
	timeout = K_TICKS_FOREVER;


	k_thread_create(&condvar_tid, stack_1, STACK_SIZE,
			condvar_wait_task, &timeout, NULL, NULL,
			PRIO_WAIT, K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	/* giving time for the condvar_wait_task to execute */
	k_yield();

	k_thread_abort(&condvar_tid);
}


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


ZTEST_USER(condvar_tests, test_condvar_wait_nowait_wake)
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
	int idx = *(int *)p2;

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
	int idx = *(int *)p2;

	zassert_true(woken_num > 0, "invalid woken number");

	if (woken > 1) {
		ret_value = k_condvar_signal(&multiple_condvar[idx]);
	} else {
		ret_value = k_condvar_broadcast(&multiple_condvar[idx]);
	}

	zassert_true(ret_value == woken_num, "k_condvar_wake failed. (%d!=%d)",
		     ret_value, woken_num);
}

ZTEST_USER(condvar_tests, test_multiple_condvar_wait_wake)
{
	woken = 1;
	timeout = K_TICKS_FOREVER;

	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		index[i] = i;

		k_thread_create(&multiple_tid[i], multiple_stack[i],
				STACK_SIZE, condvar_multiple_wait_wake_task,
				&timeout, &index[i], NULL, PRIO_WAIT,
				K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	}

	/* giving time for the other threads to execute */
	k_msleep(10);

	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		k_thread_create(&multiple_wake_tid[i], multiple_wake_stack[i],
				STACK_SIZE, condvar_multiple_wake_task,
				&woken, &index[i], NULL, PRIO_WAKE,
				K_USER | K_INHERIT_PERMS, K_NO_WAIT);
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

ZTEST_USER(condvar_tests, test_condvar_signal_null)
{
	k_tid_t tid = k_thread_create(&condvar_tid, stack_1, STACK_SIZE,
			cond_signal_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(0),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	k_thread_join(tid, K_FOREVER);
}
ZTEST_USER(condvar_tests, test_condvar_broadcast_null)
{
	k_tid_t tid = k_thread_create(&condvar_tid, stack_1, STACK_SIZE,
			cond_broadcast_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(0),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}
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

ZTEST_USER(condvar_tests, test_condvar_usecase_signal)
{
	_condvar_usecase(0);
}

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

ZTEST_SUITE(condvar_tests, NULL, condvar_tests_setup, NULL, NULL, NULL);
