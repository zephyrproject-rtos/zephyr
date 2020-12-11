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
	uint32_t msg;
};

#define SIGNAL_RESULT 0x1ee7d00d
#define FIFO_MSG_VALUE 0xdeadbeef
#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACKSIZE)

/* verify k_poll() without waiting */
static struct k_sem no_wait_sem;
static struct k_fifo no_wait_fifo;
static struct k_poll_signal no_wait_signal;
static struct k_sem zero_events_sem;
static struct k_thread test_thread;
static struct k_thread test_loprio_thread;
K_THREAD_STACK_DEFINE(test_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(test_loprio_stack, STACK_SIZE);

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
 * k_poll_signal_raise(), k_poll_signal_check()
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

#ifdef CONFIG_USERSPACE
	/* Test that k_poll() syscall handler safely handles being
	 * fed garbage
	 *
	 * TODO: Where possible migrate these to the main k_poll()
	 * implementation
	 */

	zassert_equal(k_poll(events, INT_MAX, K_NO_WAIT), -EINVAL, NULL);
	zassert_equal(k_poll(events, 4096, K_NO_WAIT), -ENOMEM, NULL);

	/* Allow zero events */
	zassert_equal(k_poll(events, 0, K_NO_WAIT), -EAGAIN, NULL);

	struct k_poll_event bad_events[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE,
					 K_POLL_NUM_MODES,
					 &no_wait_sem),
	};
	zassert_equal(k_poll(bad_events, ARRAY_SIZE(bad_events), K_NO_WAIT),
		      -EINVAL,
		      NULL);

	struct k_poll_event bad_events2[] = {
		K_POLL_EVENT_INITIALIZER(0xFU,
					 K_POLL_MODE_NOTIFY_ONLY,
					 &no_wait_sem),
	};
	zassert_equal(k_poll(bad_events2, ARRAY_SIZE(bad_events), K_NO_WAIT),
		      -EINVAL,
		      NULL);
#endif /* CONFIG_USERSPACE */

	/* test polling events that are already ready */
	zassert_false(k_fifo_alloc_put(&no_wait_fifo, &msg), NULL);
	k_poll_signal_raise(&no_wait_signal, SIGNAL_RESULT);

	zassert_equal(k_poll(events, ARRAY_SIZE(events), K_NO_WAIT), 0, "");

	zassert_equal(events[0].state, K_POLL_STATE_SEM_AVAILABLE, "");
	zassert_equal(k_sem_take(&no_wait_sem, K_NO_WAIT), 0, "");

	zassert_equal(events[1].state, K_POLL_STATE_FIFO_DATA_AVAILABLE, "");
	msg_ptr = k_fifo_get(&no_wait_fifo, K_NO_WAIT);
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

	zassert_equal(k_poll(events, ARRAY_SIZE(events), K_NO_WAIT), -EAGAIN,
		      "");
	zassert_equal(events[0].state, K_POLL_STATE_NOT_READY, "");
	zassert_equal(events[1].state, K_POLL_STATE_NOT_READY, "");
	zassert_equal(events[2].state, K_POLL_STATE_NOT_READY, "");
	zassert_equal(events[3].state, K_POLL_STATE_NOT_READY, "");

	zassert_not_equal(k_sem_take(&no_wait_sem, K_NO_WAIT), 0, "");
	zassert_is_null(k_fifo_get(&no_wait_fifo, K_NO_WAIT), "");
}

/* verify k_poll() that has to wait */

static K_SEM_DEFINE(wait_sem, 0, 1);
static K_FIFO_DEFINE(wait_fifo);
static struct k_poll_signal wait_signal =
	K_POLL_SIGNAL_INITIALIZER(wait_signal);

struct fifo_msg wait_msg = { NULL, FIFO_MSG_VALUE };

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

	k_sleep(K_MSEC(250));

	k_sem_give(&wait_sem);

	if ((intptr_t)use_fifo) {
		k_fifo_alloc_put(&wait_fifo, &wait_msg);
	}

	k_poll_signal_raise(&wait_signal, SIGNAL_RESULT);
}

/* check results for multiple events */
void check_results(struct k_poll_event *events, uint32_t event_type,
		bool is_available)
{
	struct fifo_msg *msg_ptr;

	switch (event_type) {
	case K_POLL_TYPE_SEM_AVAILABLE:
		if (is_available) {
			zassert_equal(events->state, K_POLL_STATE_SEM_AVAILABLE,
					"");
			zassert_equal(k_sem_take(&wait_sem, K_NO_WAIT), 0, "");
			zassert_equal(events->tag, TAG_0, "");
			/* reset to not ready */
			events->state = K_POLL_STATE_NOT_READY;
		} else {
			zassert_equal(events->state, K_POLL_STATE_NOT_READY,
					"");
			zassert_equal(k_sem_take(&wait_sem, K_NO_WAIT), -EBUSY,
					"");
			zassert_equal(events->tag, TAG_0, "");
		}
		break;
	case K_POLL_TYPE_DATA_AVAILABLE:
		if (is_available) {
			zassert_equal(events->state,
					K_POLL_STATE_FIFO_DATA_AVAILABLE, "");
			msg_ptr = k_fifo_get(&wait_fifo, K_NO_WAIT);
			zassert_not_null(msg_ptr, "");
			zassert_equal(msg_ptr, &wait_msg, "");
			zassert_equal(msg_ptr->msg, FIFO_MSG_VALUE, "");
			zassert_equal(events->tag, TAG_1, "");
			/* reset to not ready */
			events->state = K_POLL_STATE_NOT_READY;
		} else {
			zassert_equal(events->state, K_POLL_STATE_NOT_READY,
					"");
		}
		break;
	case K_POLL_TYPE_SIGNAL:
		if (is_available) {
			zassert_equal(wait_events[2].state,
					K_POLL_STATE_SIGNALED, "");
			zassert_equal(wait_signal.signaled, 1, "");
			zassert_equal(wait_signal.result, SIGNAL_RESULT, "");
			zassert_equal(wait_events[2].tag, TAG_2, "");
			/* reset to not ready */
			events->state = K_POLL_STATE_NOT_READY;
			wait_signal.signaled = 0U;
		} else {
			zassert_equal(events->state, K_POLL_STATE_NOT_READY,
					"");
		}
		break;
	case K_POLL_TYPE_IGNORE:
		zassert_equal(wait_events[3].state, K_POLL_STATE_NOT_READY, "");
		break;
	default:
		__ASSERT(false, "invalid event type (0x%x)\n", event_type);
		break;
	}
}

/**
 * @brief Test polling with wait
 *
 * @ingroup kernel_poll_tests
 *
 * @details
 * Test Objective:
 * - Test the poll operation which enables waiting concurrently
 * for one/two/all conditions to be fulfilled.
 * - set a single timeout argument indicating
 * the maximum amount of time a thread shall wait.
 *
 * Testing techniques:
 * - function and block box testing.
 * - Interface testing.
 * - Dynamic analysis and testing.
 *
 * Prerequisite Conditions:
 * - CONFIG_TEST_USERSPACE
 * - CONFIG_DYNAMIC_OBJECTS
 * - CONFIG_POLL
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Use FIFO/semaphore/signal object to define poll event.
 * -# Initialize the FIFO/semaphore/signal object.
 * -# Create a thread to put FIFO,
 * give semaphore and raise signal.
 * -# Check the result when signal is raised,
 * semaphore is givn and fifo is filled.
 * -# Check the result when no event is satisfied.
 * -# Check the result when only semaphore is given.
 * -# Check the result when only FIFO is filled.
 * -# Check the result when only signal is raised.
 *
 * Expected Test Result:
 * - FIFO/semaphore/signal events available/waitable in poll.
 *
 * Pass/Fail Criteria:
 * - Successful if check points in test procedure are all passed, otherwise failure.
 *
 * Assumptions and Constraints:
 * - N/A
 *
 * @see k_poll_signal_init(), k_poll()
 */
void test_poll_wait(void)
{
	const int main_low_prio = 10;

	int rc;

	int old_prio = k_thread_priority_get(k_current_get());

	k_poll_signal_init(&wait_signal);
	/*
	 * Wait for 3 non-ready events to become ready from a higher priority
	 * thread.
	 */
	k_thread_priority_set(k_current_get(), main_low_prio);

	k_thread_create(&test_thread, test_stack,
			K_THREAD_STACK_SIZEOF(test_stack),
			poll_wait_helper, (void *)1, 0, 0,
			main_low_prio - 1, K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	rc = k_poll(wait_events, ARRAY_SIZE(wait_events), K_NO_WAIT);
	zassert_equal(rc, -EAGAIN, "should return EAGAIN with K_NO_WAIT");

	rc = k_poll(wait_events, ARRAY_SIZE(wait_events), K_SECONDS(1));

	k_thread_priority_set(k_current_get(), old_prio);

	zassert_equal(rc, 0, "");
	/* all events should be available. */
	check_results(&wait_events[0], K_POLL_TYPE_SEM_AVAILABLE, true);
	check_results(&wait_events[1], K_POLL_TYPE_DATA_AVAILABLE, true);
	check_results(&wait_events[2], K_POLL_TYPE_SIGNAL, true);
	check_results(&wait_events[3], K_POLL_TYPE_IGNORE, true);

	/* verify events are not ready anymore */
	zassert_equal(k_poll(wait_events, ARRAY_SIZE(wait_events),
			     K_SECONDS(1)), -EAGAIN, "");
	/* all events should not be available. */
	check_results(&wait_events[0], K_POLL_TYPE_SEM_AVAILABLE, false);
	check_results(&wait_events[1], K_POLL_TYPE_DATA_AVAILABLE, false);
	check_results(&wait_events[2], K_POLL_TYPE_SIGNAL, false);
	check_results(&wait_events[3], K_POLL_TYPE_IGNORE, false);

	/*
	 * Wait for 2 out of 3 non-ready events to become ready from a higher
	 * priority thread.
	 */
	k_thread_priority_set(k_current_get(), main_low_prio);

	k_thread_create(&test_thread, test_stack,
			K_THREAD_STACK_SIZEOF(test_stack),
			poll_wait_helper,
			0, 0, 0, main_low_prio - 1, 0, K_NO_WAIT);

	rc = k_poll(wait_events, ARRAY_SIZE(wait_events), K_SECONDS(1));

	k_thread_priority_set(k_current_get(), old_prio);

	zassert_equal(rc, 0, "");

	check_results(&wait_events[0], K_POLL_TYPE_SEM_AVAILABLE, true);
	check_results(&wait_events[1], K_POLL_TYPE_DATA_AVAILABLE, false);
	check_results(&wait_events[2], K_POLL_TYPE_SIGNAL, true);

	/*
	 * Wait for each event to be ready from a lower priority thread, one at
	 * a time.
	 */
	k_thread_create(&test_thread, test_stack,
			K_THREAD_STACK_SIZEOF(test_stack),
			poll_wait_helper,
			(void *)1, 0, 0, old_prio + 1,
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	/* semaphore */
	rc = k_poll(wait_events, ARRAY_SIZE(wait_events), K_SECONDS(1));

	zassert_equal(rc, 0, "");

	check_results(&wait_events[0], K_POLL_TYPE_SEM_AVAILABLE, true);
	check_results(&wait_events[1], K_POLL_TYPE_DATA_AVAILABLE, false);
	check_results(&wait_events[2], K_POLL_TYPE_SIGNAL, false);

	/* fifo */
	rc = k_poll(wait_events, ARRAY_SIZE(wait_events), K_SECONDS(1));

	zassert_equal(rc, 0, "");

	check_results(&wait_events[0], K_POLL_TYPE_SEM_AVAILABLE, false);
	check_results(&wait_events[1], K_POLL_TYPE_DATA_AVAILABLE, true);
	check_results(&wait_events[2], K_POLL_TYPE_SIGNAL, false);

	/* poll signal */
	rc = k_poll(wait_events, ARRAY_SIZE(wait_events), K_SECONDS(1));

	zassert_equal(rc, 0, "");

	check_results(&wait_events[0], K_POLL_TYPE_SEM_AVAILABLE, false);
	check_results(&wait_events[1], K_POLL_TYPE_DATA_AVAILABLE, false);
	check_results(&wait_events[2], K_POLL_TYPE_SIGNAL, true);
}

/* verify k_poll() that waits on object which gets cancellation */

static struct k_fifo cancel_fifo;
static struct k_fifo non_cancel_fifo;

static void poll_cancel_helper(void *p1, void *p2, void *p3)
{
	(void)p1; (void)p2; (void)p3;

	static struct fifo_msg msg;

	k_sleep(K_MSEC(100));

	k_fifo_cancel_wait(&cancel_fifo);

	k_fifo_alloc_put(&non_cancel_fifo, &msg);
}

/**
 * @brief Test polling of cancelled fifo
 *
 * @details Test the FIFO(queue) data available/cancelable events
 * as events in poll.
 *
 * @ingroup kernel_poll_tests
 *
 * @see k_poll(), k_fifo_cancel_wait(), k_fifo_alloc_put
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

	k_thread_create(&test_thread, test_stack,
			K_THREAD_STACK_SIZEOF(test_stack),
			poll_cancel_helper, (void *)1, 0, 0,
			main_low_prio - 1, K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

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
 * @details
 * - Test the multiple semaphore events as waitable events in poll.
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

	k_thread_create(&test_thread, test_stack,
			K_THREAD_STACK_SIZEOF(test_stack),
			multi, 0, 0, 0, main_low_prio - 1,
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	/*
	 * create additional thread to add multiple(more than one)
	 * pending threads in events list to improve code coverage.
	 */
	k_thread_create(&test_loprio_thread, test_loprio_stack,
			K_THREAD_STACK_SIZEOF(test_loprio_stack),
			multi_lowprio, 0, 0, 0, main_low_prio + 1,
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	/* Allow lower priority thread to add poll event in the list */
	k_sleep(K_MSEC(250));
	rc = k_poll(events, ARRAY_SIZE(events), K_SECONDS(1));

	zassert_equal(rc, 0, "");
	zassert_equal(events[0].state, K_POLL_STATE_NOT_READY, "");
	zassert_equal(events[1].state, K_POLL_STATE_SEM_AVAILABLE, "");

	/*
	 * free polling threads, ensuring it awoken from k_poll()
	 * and got the sem
	 */
	k_sem_give(&multi_sem);
	k_sem_give(&multi_sem);
	rc = k_sem_take(&multi_reply, K_SECONDS(1));

	zassert_equal(rc, 0, "");

	/* wait for polling threads to complete execution */
	k_thread_priority_set(k_current_get(), old_prio);
	k_sleep(K_MSEC(250));
}

static struct k_poll_signal signal;

static void threadstate(void *p1, void *p2, void *p3)
{
	(void)p2; (void)p3;

	k_sleep(K_MSEC(250));
	/* Update polling thread state explicitly to improve code coverage */
	k_thread_suspend(p1);
	/* Enable polling thread by signalling */
	k_poll_signal_raise(&signal, SIGNAL_RESULT);
	k_thread_resume(p1);
}

/**
 * @brief Test polling of events by manipulating polling thread state
 *
 * @details
 * - manipulating thread state to consider case where no polling thread
 * is available during event signalling.
 * - defined a signal poll as waitable events in poll and
 * verify the result after siganl raised
 *
 * @ingroup kernel_poll_tests
 *
 * @see K_POLL_EVENT_INITIALIZER(), k_poll(), k_poll_signal_init(),
 * k_poll_signal_check(), k_poll_signal_raise()
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

	k_thread_create(&test_thread, test_stack,
			K_THREAD_STACK_SIZEOF(test_stack), threadstate,
			ztest_tid, 0, 0, main_low_prio - 1, K_INHERIT_PERMS,
			K_NO_WAIT);

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
			      &wait_signal, &test_thread,
			      &test_stack, &multi_sem, &multi_reply);
}

void test_poll_zero_events(void)
{
	struct k_poll_event event;

	k_sem_init(&zero_events_sem, 1, 1);

	k_poll_event_init(&event, K_POLL_TYPE_SEM_AVAILABLE,
			  K_POLL_MODE_NOTIFY_ONLY, &zero_events_sem);

	zassert_equal(k_poll(&event, 0, K_MSEC(50)), -EAGAIN, NULL);
}
