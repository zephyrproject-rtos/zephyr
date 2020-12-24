/*
 * Copyright (c) 2021 intel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <kernel.h>
#include <ztest_error_hook.h>

static struct k_poll_signal signal_err;
#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACKSIZE)
static struct k_thread test_thread1;
static struct k_thread test_thread2;
K_THREAD_STACK_DEFINE(test_stack1, STACK_SIZE);
K_THREAD_STACK_DEFINE(test_stack2, STACK_SIZE);

/**
 * @brief Test API k_poll with error events type in kernel mode
 *
 * @details Define a poll event and initialize by k_poll_event_init(), and using
 * API k_poll with error events type as parameter check if a error will be met.
 *
 * @see k_poll()
 *
 * @ingroup kernel_poll_tests
 */
void test_condition_met_type_err(void)
{
	struct k_poll_event event;
	struct k_fifo fifo;

	ztest_set_assert_valid(true);
	k_fifo_init(&fifo);
	k_poll_event_init(&event, K_POLL_TYPE_DATA_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY, &fifo);

	event.type = 5;
	k_poll(&event, 1, K_NO_WAIT);
}

/* verify multiple pollers */
static K_SEM_DEFINE(multi_sem, 0, 1);
static K_SEM_DEFINE(multi_ready_sem, 1, 1);
static K_SEM_DEFINE(multi_reply, 0, 1);

static void thread_entry(void *p1, void *p2, void *p3)
{
	struct k_poll_event event;

	k_poll_event_init(&event, K_POLL_TYPE_SEM_AVAILABLE,
			  K_POLL_MODE_NOTIFY_ONLY, &multi_sem);

	(void)k_poll(&event, 1, K_FOREVER);
	k_sem_take(&multi_sem, K_FOREVER);
	k_sem_give(&multi_reply);
}

/**
 * @brief Test polling of multiple events by lower priority thread
 *
 * @details
 * - Test the multiple semaphore events as waitable events in poll.
 *
 * @ingroup kernel_poll_tests
 *
 * @see K_POLL_EVENT_INITIALIZER(), k_poll(), k_poll_event_init()
 */
void test_poll_lower_prio(void)
{
	int old_prio = k_thread_priority_get(k_current_get());
	struct k_thread *p = &test_thread1;
	const int main_low_prio = 10;
	const int low_prio_than_main = 11;
	int rc;

	struct k_poll_event events[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE,
					 K_POLL_MODE_NOTIFY_ONLY,
					 &multi_sem),
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE,
					 K_POLL_MODE_NOTIFY_ONLY,
					 &multi_ready_sem),
	};

	k_thread_priority_set(k_current_get(), main_low_prio);

	k_thread_create(&test_thread1, test_stack1,
			K_THREAD_STACK_SIZEOF(test_stack1),
			thread_entry, 0, 0, 0, low_prio_than_main,
			K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_create(&test_thread2, test_stack2,
			K_THREAD_STACK_SIZEOF(test_stack2),
			thread_entry, 0, 0, 0, low_prio_than_main,
			K_INHERIT_PERMS, K_NO_WAIT);

	/* Set up the thread timeout value to check if what happened if dticks is invalid */
	p->base.timeout.dticks = _EXPIRED;

	/* Delay for some actions above */
	k_sleep(K_MSEC(250));
	(void)k_poll(events, ARRAY_SIZE(events), K_SECONDS(1));

	k_sem_give(&multi_sem);
	k_sem_give(&multi_sem);
	rc = k_sem_take(&multi_reply, K_FOREVER);

	zassert_equal(rc, 0, "");

	/* Reset the initialized state */
	k_thread_priority_set(k_current_get(), old_prio);
	k_sleep(K_MSEC(250));
}

#ifdef CONFIG_USERSPACE
/**
 * @brief Test API k_poll with error number events in user mode
 *
 * @details Using API k_poll with error number
 * as parameter to check if a error will be met.
 *
 * @see k_poll()
 *
 * @ingroup kernel_poll_tests
 */
void test_k_poll_user_num_err(void)
{
	struct k_poll_event events;

	k_poll(&events, -1, K_NO_WAIT);
}

/**
 * @brief Test API k_poll with error member of events in user mode
 *
 * @details Using API k_poll with error member
 * as parameter to check if a error will be met.
 *
 * @see k_poll()
 *
 * @ingroup kernel_poll_tests
 */
void test_k_poll_user_mem_err(void)
{
	ztest_set_fault_valid(true);
	k_poll(NULL, 3, K_NO_WAIT);
}

/**
 * @brief Test API k_poll with NULL sem event in user mode
 *
 * @details Define a poll event, and using API k_poll with NULL sem
 * as parameter to check if a error will be met.
 *
 * @see k_poll()
 *
 * @ingroup kernel_poll_tests
 */
void test_k_poll_user_type_sem_err(void)
{
	struct k_poll_event event[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE,
					 K_POLL_MODE_NOTIFY_ONLY,
					 NULL),
	};

	ztest_set_fault_valid(true);
	k_poll(event, 1, K_NO_WAIT);
}

/**
 * @brief Test API k_poll with NULL signal event in user mode
 *
 * @details Define a poll, and using API k_poll with NULL signal
 * as parameter to check if a error will be met.
 *
 * @see k_poll()
 *
 * @ingroup kernel_poll_tests
 */
void test_k_poll_user_type_signal_err(void)
{
	struct k_poll_event event[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
					 K_POLL_MODE_NOTIFY_ONLY,
					 NULL),
	};

	ztest_set_fault_valid(true);
	k_poll(event, 1, K_NO_WAIT);
}

/**
 * @brief Test API k_poll with NULL fifo event in user mode
 *
 * @details Define a poll, and using API k_poll with NULL fifo
 * as parameter to check if a error will be met.
 *
 * @see k_poll()
 *
 * @ingroup kernel_poll_tests
 */
void test_k_poll_user_type_fifo_err(void)
{
	struct k_poll_event event[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_FIFO_DATA_AVAILABLE,
					 K_POLL_MODE_NOTIFY_ONLY,
					 NULL),
	};

	ztest_set_fault_valid(true);
	k_poll(event, 1, K_NO_WAIT);
}

/**
 * @brief Test API k_poll_signal_init with NULL in user mode
 *
 * @details Using API k_poll_signal_init with NULL as
 * parameter to check if a error will be met.
 *
 * @see k_poll_signal_init()
 *
 * @ingroup kernel_poll_tests
 */
void test_poll_signal_init_null(void)
{
	ztest_set_fault_valid(true);
	k_poll_signal_init(NULL);
}

/**
 * @brief Test API k_poll_signal_check with NULL object in user mode
 *
 * @details Using API k_poll with NULL object
 * as parameter to check if a error will be met.
 *
 * @see k_poll_signal_check()
 *
 * @ingroup kernel_poll_tests
 */
void test_poll_signal_check_obj(void)
{
	unsigned int signaled;
	int result;

	ztest_set_fault_valid(true);
	k_poll_signal_check(NULL, &signaled, &result);
}

/**
 * @brief Test API k_poll_signal_check with unread address
 * in user mode
 *
 * @details Using k_poll_signal_check with
 * unread results as parameter to check if a error
 * will be met.
 *
 * @see k_poll_signal_check()
 *
 * @ingroup kernel_poll_tests
 */
void test_poll_signal_check_signal(void)
{
	unsigned int result;

	k_poll_signal_init(&signal_err);

	ztest_set_fault_valid(true);
	k_poll_signal_check(&signal_err, NULL, &result);
}



/**
 * @brief Test API k_poll_signal_check with unread address
 * in user mode
 *
 * @details Using k_poll_signal_check with
 * unread signaled as parameter to check if a error
 * will be met.
 *
 * @see k_poll_signal_check()
 *
 * @ingroup kernel_poll_tests
 */
void test_poll_signal_check_result(void)
{
	int signaled;

	k_poll_signal_init(&signal_err);

	ztest_set_fault_valid(true);
	k_poll_signal_check(&signal_err, &signaled, NULL);
}

/**
 * @brief Test API k_poll_signal_raise with unread address
 * in user mode
 *
 * @details Using k_poll_signal_raise with
 * NULL as parameter to check if a error
 * will be met.
 *
 * @see k_poll_signal_raise()
 *
 * @ingroup kernel_poll_tests
 */
void test_poll_signal_raise_null(void)
{
	int result = 0;

	ztest_set_fault_valid(true);
	k_poll_signal_raise(NULL, result);
}

/**
 * @brief Test API k_poll_signal_reset with unread address
 * in user mode
 *
 * @details Using k_poll_signal_reset with
 * NULL as parameter to check if a error
 * will be met.
 *
 * @see k_poll_signal_reset()
 *
 * @ingroup kernel_poll_tests
 */
void test_poll_signal_reset_null(void)
{
	ztest_set_fault_valid(true);
	k_poll_signal_reset(NULL);
}
#endif /* CONFIG_USERSPACE */

void test_poll_fail_grant_access(void)
{
	k_thread_access_grant(k_current_get(), &signal_err);
}
