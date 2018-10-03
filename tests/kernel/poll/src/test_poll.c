/*
 * Copyright (c) 2017 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <kernel.h>

/* global values and data structures */
struct fifo_msg {
	void *private;
	u32_t msg;
};

#define SIGNAL_RESULT 0x1ee7d00d
#define FIFO_MSG_VALUE 0xdeadbeef

/* verify k_poll() without waiting */
static __kernel struct k_sem no_wait_sem;
static __kernel struct k_fifo no_wait_fifo;
static __kernel struct k_poll_signal no_wait_signal;

/**
 * @brief Test cases to verify poll
 *
 * @defgroup kernel_poll_tests Poll tests
 *
 * @ingroup all_tests
 *
 * @{
 * @}
 */

/**
 * @brief Test poll events with no wait
 *
 * @ingroup kernel_poll_tests
 *
 * @see K_POLL_EVENT_INITIALIZER(), k_poll_signal_init(),
 * k_poll_signal(), k_poll_signal_check()
 */
void test_poll_no_wait(void)
{
	struct fifo_msg msg = { NULL, FIFO_MSG_VALUE }, *msg_ptr;
	unsigned int signaled;
	int result;

	k_sem_init(&no_wait_sem, 1, 1);
	k_fifo_init(&no_wait_fifo);
	k_poll_signal_init(&no_wait_signal);

	struct k_poll_event events[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE,
					 K_POLL_MODE_NOTIFY_ONLY,
					 &no_wait_sem),
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_FIFO_DATA_AVAILABLE,
					 K_POLL_MODE_NOTIFY_ONLY,
					 &no_wait_fifo),
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
					 K_POLL_MODE_NOTIFY_ONLY,
					 &no_wait_signal),
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_IGNORE,
					 K_POLL_MODE_NOTIFY_ONLY,
					 NULL),
	};

	/* test polling events that are already ready */
	zassert_false(k_fifo_alloc_put(&no_wait_fifo, &msg), NULL);
	k_poll_signal(&no_wait_signal, SIGNAL_RESULT);

	zassert_equal(k_poll(events, ARRAY_SIZE(events), 0), 0, "");

	zassert_equal(events[0].state, K_POLL_STATE_SEM_AVAILABLE, "");
	zassert_equal(k_sem_take(&no_wait_sem, 0), 0, "");

	zassert_equal(events[1].state, K_POLL_STATE_FIFO_DATA_AVAILABLE, "");
	msg_ptr = k_fifo_get(&no_wait_fifo, 0);
	zassert_not_null(msg_ptr, "");
	zassert_equal(msg_ptr, &msg, "");
	zassert_equal(msg_ptr->msg, FIFO_MSG_VALUE, "");

	zassert_equal(events[2].state, K_POLL_STATE_SIGNALED, "");
	k_poll_signal_check(&no_wait_signal, &signaled, &result);
	zassert_not_equal(signaled, 0, "");
	zassert_equal(result, SIGNAL_RESULT, "");

	zassert_equal(events[3].state, K_POLL_STATE_NOT_READY, "");

	/* verify events are not ready anymore (user has to clear them first) */
	events[0].state = K_POLL_STATE_NOT_READY;
	events[1].state = K_POLL_STATE_NOT_READY;
	events[2].state = K_POLL_STATE_NOT_READY;
	events[3].state = K_POLL_STATE_NOT_READY;
	k_poll_signal_reset(&no_wait_signal);

	zassert_equal(k_poll(events, ARRAY_SIZE(events), 0), -EAGAIN, "");
	zassert_equal(events[0].state, K_POLL_STATE_NOT_READY, "");
	zassert_equal(events[1].state, K_POLL_STATE_NOT_READY, "");
	zassert_equal(events[2].state, K_POLL_STATE_NOT_READY, "");
	zassert_equal(events[3].state, K_POLL_STATE_NOT_READY, "");

	zassert_not_equal(k_sem_take(&no_wait_sem, 0), 0, "");
	zassert_is_null(k_fifo_get(&no_wait_fifo, 0), "");
}

/* verify k_poll() that has to wait */

static K_SEM_DEFINE(wait_sem, 0, 1);
static K_FIFO_DEFINE(wait_fifo);
static __kernel struct k_poll_signal wait_signal =
	K_POLL_SIGNAL_INITIALIZER(wait_signal);

struct fifo_msg wait_msg = { NULL, FIFO_MSG_VALUE };

static __kernel struct k_thread poll_wait_helper_thread;
static K_THREAD_STACK_DEFINE(poll_wait_helper_stack, KB(1));

#define TAG_0 10
#define TAG_1 11
#define TAG_2 12

struct k_poll_event wait_events[] = {
	K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE,
					K_POLL_MODE_NOTIFY_ONLY,
					&wait_sem, TAG_0),
	K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_FIFO_DATA_AVAILABLE,
					K_POLL_MODE_NOTIFY_ONLY,
					&wait_fifo, TAG_1),
	K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SIGNAL,
					K_POLL_MODE_NOTIFY_ONLY,
					&wait_signal, TAG_2),
	K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_IGNORE,
					K_POLL_MODE_NOTIFY_ONLY,
					NULL),
};

static void poll_wait_helper(void *use_fifo, void *p2, void *p3)
{
	(void)p2; (void)p3;

	k_sleep(250);

	k_sem_give(&wait_sem);

	if ((intptr_t)use_fifo) {
		k_fifo_alloc_put(&wait_fifo, &wait_msg);
	}

	k_poll_signal(&wait_signal, SIGNAL_RESULT);
}

/**
 * @brief Test polling with wait
 *
 * @ingroup kernel_poll_tests
 *
 * @see k_poll_signal_init(), k_poll()
 */
void test_poll_wait(void)
{
	const int main_low_prio = 10;

	struct fifo_msg *msg_ptr;
	int rc;

	int old_prio = k_thread_priority_get(k_current_get());

	k_poll_signal_init(&wait_signal);
	/*
	 * Wait for 3 non-ready events to become ready from a higher priority
	 * thread.
	 */
	k_thread_priority_set(k_current_get(), main_low_prio);

	k_thread_create(&poll_wait_helper_thread, poll_wait_helper_stack,
			K_THREAD_STACK_SIZEOF(poll_wait_helper_stack),
			poll_wait_helper, (void *)1, 0, 0,
			main_low_prio - 1, K_USER | K_INHERIT_PERMS, 0);

	rc = k_poll(wait_events, ARRAY_SIZE(wait_events), K_SECONDS(1));

	k_thread_priority_set(k_current_get(), old_prio);

	zassert_equal(rc, 0, "");

	zassert_equal(wait_events[0].state, K_POLL_STATE_SEM_AVAILABLE, "");
	zassert_equal(k_sem_take(&wait_sem, 0), 0, "");
	zassert_equal(wait_events[0].tag, TAG_0, "");

	zassert_equal(wait_events[1].state,
		      K_POLL_STATE_FIFO_DATA_AVAILABLE, "");
	msg_ptr = k_fifo_get(&wait_fifo, 0);
	zassert_not_null(msg_ptr, "");
	zassert_equal(msg_ptr, &wait_msg, "");
	zassert_equal(msg_ptr->msg, FIFO_MSG_VALUE, "");
	zassert_equal(wait_events[1].tag, TAG_1, "");

	zassert_equal(wait_events[2].state, K_POLL_STATE_SIGNALED, "");
	zassert_equal(wait_signal.signaled, 1, "");
	zassert_equal(wait_signal.result, SIGNAL_RESULT, "");
	zassert_equal(wait_events[2].tag, TAG_2, "");

	zassert_equal(wait_events[3].state, K_POLL_STATE_NOT_READY, "");

	/* verify events are not ready anymore */
	wait_events[0].state = K_POLL_STATE_NOT_READY;
	wait_events[1].state = K_POLL_STATE_NOT_READY;
	wait_events[2].state = K_POLL_STATE_NOT_READY;
	wait_events[3].state = K_POLL_STATE_NOT_READY;
	wait_signal.signaled = 0;

	zassert_equal(k_poll(wait_events, ARRAY_SIZE(wait_events),
			     K_SECONDS(1)), -EAGAIN, "");

	zassert_equal(wait_events[0].state, K_POLL_STATE_NOT_READY, "");
	zassert_equal(wait_events[1].state, K_POLL_STATE_NOT_READY, "");
	zassert_equal(wait_events[2].state, K_POLL_STATE_NOT_READY, "");
	zassert_equal(wait_events[3].state, K_POLL_STATE_NOT_READY, "");

	/* tags should not have been touched */
	zassert_equal(wait_events[0].tag, TAG_0, "");
	zassert_equal(wait_events[1].tag, TAG_1, "");
	zassert_equal(wait_events[2].tag, TAG_2, "");

	/*
	 * Wait for 2 out of 3 non-ready events to become ready from a higher
	 * priority thread.
	 */
	k_thread_priority_set(k_current_get(), main_low_prio);

	k_thread_create(&poll_wait_helper_thread, poll_wait_helper_stack,
			K_THREAD_STACK_SIZEOF(poll_wait_helper_stack),
			poll_wait_helper,
			0, 0, 0, main_low_prio - 1, 0, 0);

	rc = k_poll(wait_events, ARRAY_SIZE(wait_events), K_SECONDS(1));

	k_thread_priority_set(k_current_get(), old_prio);

	zassert_equal(rc, 0, "");

	zassert_equal(wait_events[0].state, K_POLL_STATE_SEM_AVAILABLE, "");
	zassert_equal(k_sem_take(&wait_sem, 0), 0, "");
	zassert_equal(wait_events[0].tag, TAG_0, "");

	zassert_equal(wait_events[1].state, K_POLL_STATE_NOT_READY, "");
	msg_ptr = k_fifo_get(&wait_fifo, K_NO_WAIT);
	zassert_is_null(msg_ptr, "");
	zassert_equal(wait_events[1].tag, TAG_1, "");

	zassert_equal(wait_events[2].state, K_POLL_STATE_SIGNALED, "");
	zassert_equal(wait_signal.signaled, 1, "");
	zassert_equal(wait_signal.result, SIGNAL_RESULT, "");
	zassert_equal(wait_events[2].tag, TAG_2, "");

	/*
	 * Wait for each event to be ready from a lower priority thread, one at
	 * a time.
	 */

	wait_events[0].state = K_POLL_STATE_NOT_READY;
	wait_events[1].state = K_POLL_STATE_NOT_READY;
	wait_events[2].state = K_POLL_STATE_NOT_READY;
	wait_signal.signaled = 0;

	k_thread_create(&poll_wait_helper_thread, poll_wait_helper_stack,
			K_THREAD_STACK_SIZEOF(poll_wait_helper_stack),
			poll_wait_helper,
			(void *)1, 0, 0, old_prio + 1,
			K_USER | K_INHERIT_PERMS, 0);

	/* semaphore */
	rc = k_poll(wait_events, ARRAY_SIZE(wait_events), K_SECONDS(1));

	zassert_equal(rc, 0, "");

	zassert_equal(wait_events[0].state, K_POLL_STATE_SEM_AVAILABLE, "");
	zassert_equal(k_sem_take(&wait_sem, 0), 0, "");
	zassert_equal(wait_events[0].tag, TAG_0, "");

	zassert_equal(wait_events[1].state, K_POLL_STATE_NOT_READY, "");
	msg_ptr = k_fifo_get(&wait_fifo, K_NO_WAIT);
	zassert_is_null(msg_ptr, "");
	zassert_equal(wait_events[1].tag, TAG_1, "");

	zassert_equal(wait_events[2].state, K_POLL_STATE_NOT_READY, "");
	zassert_equal(wait_events[2].tag, TAG_2, "");

	wait_events[0].state = K_POLL_STATE_NOT_READY;

	/* fifo */
	rc = k_poll(wait_events, ARRAY_SIZE(wait_events), K_SECONDS(1));

	zassert_equal(rc, 0, "");

	zassert_equal(wait_events[0].state, K_POLL_STATE_NOT_READY, "");
	zassert_equal(k_sem_take(&wait_sem, 0), -EBUSY, "");
	zassert_equal(wait_events[0].tag, TAG_0, "");

	zassert_equal(wait_events[1].state,
		      K_POLL_STATE_FIFO_DATA_AVAILABLE, "");
	msg_ptr = k_fifo_get(&wait_fifo, K_NO_WAIT);
	zassert_not_null(msg_ptr, "");
	zassert_equal(wait_events[1].tag, TAG_1, "");

	zassert_equal(wait_events[2].state, K_POLL_STATE_NOT_READY, "");
	zassert_equal(wait_events[2].tag, TAG_2, "");

	wait_events[1].state = K_POLL_STATE_NOT_READY;

	/* poll signal */
	rc = k_poll(wait_events, ARRAY_SIZE(wait_events), K_SECONDS(1));

	zassert_equal(rc, 0, "");

	zassert_equal(wait_events[0].state, K_POLL_STATE_NOT_READY, "");
	zassert_equal(k_sem_take(&wait_sem, 0), -EBUSY, "");
	zassert_equal(wait_events[0].tag, TAG_0, "");

	zassert_equal(wait_events[1].state, K_POLL_STATE_NOT_READY, "");
	msg_ptr = k_fifo_get(&wait_fifo, K_NO_WAIT);
	zassert_is_null(msg_ptr, "");
	zassert_equal(wait_events[1].tag, TAG_1, "");

	zassert_equal(wait_events[2].state, K_POLL_STATE_SIGNALED, "");
	zassert_equal(wait_signal.signaled, 1, "");
	zassert_equal(wait_signal.result, SIGNAL_RESULT, "");
	zassert_equal(wait_events[2].tag, TAG_2, "");

	wait_events[2].state = K_POLL_STATE_NOT_READY;
	wait_signal.signaled = 0;
}

/* verify k_poll() that waits on object which gets cancellation */

static __kernel struct k_fifo cancel_fifo;
static __kernel struct k_fifo non_cancel_fifo;

static __kernel struct k_thread poll_cancel_helper_thread;
static K_THREAD_STACK_DEFINE(poll_cancel_helper_stack, 768);

static void poll_cancel_helper(void *p1, void *p2, void *p3)
{
	(void)p1; (void)p2; (void)p3;

	static struct fifo_msg msg;

	k_sleep(100);

	k_fifo_cancel_wait(&cancel_fifo);

	k_fifo_alloc_put(&non_cancel_fifo, &msg);
}

/**
 * @brief Test polling of cancelled fifo
 *
 * @ingroup kernel_poll_tests
 *
 * @see k_poll(), k_fifo_cancel_wait()
 */
void test_poll_cancel(bool is_main_low_prio)
{
	const int main_low_prio = 10;
	int old_prio = k_thread_priority_get(k_current_get());
	int rc;

	struct k_poll_event cancel_events[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_FIFO_DATA_AVAILABLE,
					K_POLL_MODE_NOTIFY_ONLY,
					&cancel_fifo),
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_FIFO_DATA_AVAILABLE,
					K_POLL_MODE_NOTIFY_ONLY,
					&non_cancel_fifo),
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_IGNORE,
					K_POLL_MODE_NOTIFY_ONLY,
					NULL),
	};

	k_fifo_init(&cancel_fifo);
	k_fifo_init(&non_cancel_fifo);

	if (is_main_low_prio) {
		k_thread_priority_set(k_current_get(), main_low_prio);
	}

	k_thread_create(&poll_cancel_helper_thread, poll_cancel_helper_stack,
			K_THREAD_STACK_SIZEOF(poll_cancel_helper_stack),
			poll_cancel_helper, (void *)1, 0, 0,
			main_low_prio - 1, K_USER | K_INHERIT_PERMS, 0);

	rc = k_poll(cancel_events, ARRAY_SIZE(cancel_events), K_SECONDS(1));

	k_thread_priority_set(k_current_get(), old_prio);

	zassert_equal(rc, -EINTR, "");

	zassert_equal(cancel_events[0].state,
		      K_POLL_STATE_CANCELLED, "");

	if (is_main_low_prio) {
		/* If poller thread is lower priority than threads which
		 * generate poll events, it may get multiple poll events
		 * at once.
		 */
		zassert_equal(cancel_events[1].state,
			      K_POLL_STATE_FIFO_DATA_AVAILABLE, "");
	} else {
		/* Otherwise, poller thread will be woken up on first
		 * event triggered.
		 */
		zassert_equal(cancel_events[1].state,
			      K_POLL_STATE_NOT_READY, "");
	}
}

void test_poll_cancel_main_low_prio(void)
{
	test_poll_cancel(true);
}

void test_poll_cancel_main_high_prio(void)
{
	test_poll_cancel(false);
}

/* verify multiple pollers */
static K_SEM_DEFINE(multi_sem, 0, 1);

static __kernel struct k_thread multi_thread_lowprio;
static K_THREAD_STACK_DEFINE(multi_stack_lowprio, KB(1));

static void multi_lowprio(void *p1, void *p2, void *p3)
{
	(void)p1; (void)p2; (void)p3;

	struct k_poll_event event;
	int rc;

	k_poll_event_init(&event, K_POLL_TYPE_SEM_AVAILABLE,
			  K_POLL_MODE_NOTIFY_ONLY, &multi_sem);

	(void)k_poll(&event, 1, K_FOREVER);
	rc = k_sem_take(&multi_sem, K_FOREVER);
	zassert_equal(rc, 0, "");
}

static K_SEM_DEFINE(multi_reply, 0, 1);

static __kernel struct k_thread multi_thread;
static K_THREAD_STACK_DEFINE(multi_stack, KB(1));

static void multi(void *p1, void *p2, void *p3)
{
	(void)p1; (void)p2; (void)p3;

	struct k_poll_event event;

	k_poll_event_init(&event, K_POLL_TYPE_SEM_AVAILABLE,
			  K_POLL_MODE_NOTIFY_ONLY, &multi_sem);

	(void)k_poll(&event, 1, K_FOREVER);
	k_sem_take(&multi_sem, K_FOREVER);
	k_sem_give(&multi_reply);
}

static K_SEM_DEFINE(multi_ready_sem, 1, 1);

/**
 * @brief Test polling of multiple events
 *
 * @ingroup kernel_poll_tests
 *
 * @see K_POLL_EVENT_INITIALIZER(), k_poll(), k_poll_event_init()
 */
void test_poll_multi(void)
{
	int old_prio = k_thread_priority_get(k_current_get());
	const int main_low_prio = 10;
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

	k_thread_create(&multi_thread, multi_stack,
			K_THREAD_STACK_SIZEOF(multi_stack),
			multi, 0, 0, 0, main_low_prio - 1,
			K_USER | K_INHERIT_PERMS, 0);

	/*
	 * create additional thread to add multiple(more than one) pending threads
	 * in events list to improve code coverage
	 */
	k_thread_create(&multi_thread_lowprio, multi_stack_lowprio,
			K_THREAD_STACK_SIZEOF(multi_stack_lowprio),
			multi_lowprio, 0, 0, 0, main_low_prio + 1,
			K_USER | K_INHERIT_PERMS, 0);

	/* Allow lower priority thread to add poll event in the list */
	k_sleep(250);
	rc = k_poll(events, ARRAY_SIZE(events), K_SECONDS(1));

	zassert_equal(rc, 0, "");
	zassert_equal(events[0].state, K_POLL_STATE_NOT_READY, "");
	zassert_equal(events[1].state, K_POLL_STATE_SEM_AVAILABLE, "");

	/* free polling threads, ensuring it awoken from k_poll() and got the sem */
	k_sem_give(&multi_sem);
	k_sem_give(&multi_sem);
	rc = k_sem_take(&multi_reply, K_SECONDS(1));

	zassert_equal(rc, 0, "");

	/* wait for polling threads to complete execution */
	k_thread_priority_set(k_current_get(), old_prio);
	k_sleep(250);
}

static __kernel struct k_thread signal_thread;
static K_THREAD_STACK_DEFINE(signal_stack, KB(1));
static __kernel struct k_poll_signal signal;

static void threadstate(void *p1, void *p2, void *p3)
{
	(void)p2; (void)p3;

	k_sleep(250);
	/* Update polling thread state explicitly to improve code coverage */
	k_thread_suspend(p1);
	/* Enable polling thread by signalling */
	k_poll_signal(&signal, SIGNAL_RESULT);
	k_thread_resume(p1);
}

/**
 * @brief Test polling of events by manipulating polling thread state
 *
 * This is required for improving code coverage by manipulating thread
 * state to consider case where no polling thread is available during
 * event signalling.
 *
 * @ingroup kernel_poll_tests
 *
 * @see K_POLL_EVENT_INITIALIZER(), k_poll(), k_poll_signal_init(),
 * k_poll_signal_check(), k_poll_signal()
 */
void test_poll_threadstate(void)
{
	unsigned int signaled;
	const int main_low_prio = 10;
	int result;

	k_poll_signal_init(&signal);

	struct k_poll_event event;

	k_poll_event_init(&event, K_POLL_TYPE_SIGNAL,
			  K_POLL_MODE_NOTIFY_ONLY, &signal);

	int old_prio = k_thread_priority_get(k_current_get());

	k_thread_priority_set(k_current_get(), main_low_prio);
	k_tid_t ztest_tid = k_current_get();

	k_thread_create(&signal_thread, signal_stack,
			K_THREAD_STACK_SIZEOF(signal_stack), threadstate,
			ztest_tid, 0, 0, main_low_prio - 1, K_INHERIT_PERMS, 0);

	/* wait for spawn thread to take action */
	zassert_equal(k_poll(&event, 1, K_SECONDS(1)), 0, "");
	zassert_equal(event.state, K_POLL_STATE_SIGNALED, "");
	k_poll_signal_check(&signal, &signaled, &result);
	zassert_not_equal(signaled, 0, "");
	zassert_equal(result, SIGNAL_RESULT, "");

	event.state = K_POLL_STATE_NOT_READY;
	k_poll_signal_reset(&signal);
	/* teardown */
	k_thread_priority_set(k_current_get(), old_prio);
}

void test_poll_grant_access(void)
{
	k_thread_access_grant(k_current_get(), &no_wait_sem, &no_wait_fifo,
			      &no_wait_signal, &wait_sem, &wait_fifo,
			      &cancel_fifo, &non_cancel_fifo,
			      &wait_signal, &poll_wait_helper_thread,
			      &poll_wait_helper_stack, &multi_sem,
			      &multi_reply, &multi_thread, &multi_stack,
			      &multi_thread_lowprio, &multi_stack_lowprio,
			      NULL);
}
