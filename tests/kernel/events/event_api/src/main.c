/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Tests for the events kernel object
 *
 * Verify zephyr event apis under different context, both from supervisor
 * threads (white-box checks) and from user threads (system call coverage
 * when CONFIG_USERSPACE is enabled).
 *
 * - API coverage
 *   -# k_event_init K_EVENT_DEFINE
 *   -# k_event_post
 *   -# k_event_set
 *   -# k_event_set_masked
 *   -# k_event_wait
 *   -# k_event_wait_all
 *   -# k_event_wait_safe
 *   -# k_event_wait_all_safe
 *   -# k_event_test
 *
 * @defgroup kernel_event_tests Events
 * @ingroup all_tests
 * @{
 * @}
 */

#include <zephyr/ztest.h>
#include <wait_q.h>

#define DELAY          K_MSEC(50)
#define SHORT_TIMEOUT  K_MSEC(100)
#define LONG_TIMEOUT   K_MSEC(1000)

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)

static struct k_thread treceiver;
static struct k_thread textra1;
static struct k_thread textra2;

static K_THREAD_STACK_DEFINE(sreceiver, STACK_SIZE);
static K_THREAD_STACK_DEFINE(sextra1, STACK_SIZE);
static K_THREAD_STACK_DEFINE(sextra2, STACK_SIZE);

static K_EVENT_DEFINE(test_event);
static K_EVENT_DEFINE(sync_event);
static struct k_event deliver_event;
static K_EVENT_DEFINE(clear_event);

static K_SEM_DEFINE(receiver_sem, 0, 1);
static K_SEM_DEFINE(sync_sem, 0, 1);

volatile static uint32_t test_events;

/**
 * @ingroup kernel_event_tests
 * @{
 */

static void entry_extra1(void *p1, void *p2, void *p3)
{
	uint32_t  events;

	events = k_event_wait_all(&sync_event, 0x33, true, K_FOREVER);

	k_event_post(&test_event, events);
}

static void entry_extra2(void *p1, void *p2, void *p3)
{
	uint32_t  events;

	events = k_event_wait(&sync_event, 0x3300, true, K_FOREVER);

	k_event_post(&test_event, events);
}

/**
 * @brief Verify that k_event_init() produces an empty, unowned event object.
 *
 * @details
 * White-box test confirming that a freshly initialized event starts with no
 * pending events and no threads parked on its wait queue, which is the
 * precondition every other event operation relies on.
 *
 * Test steps:
 * - Initialize a k_event object with k_event_init().
 * - Read the head of the event wait queue.
 * - Read the event's set of posted events.
 *
 * Expected result:
 * - The wait queue head is NULL (no waiters).
 * - The set of posted events is 0.
 *
 * @see k_event_init()
 */
ZTEST(events_api, test_k_event_init)
{
	static struct k_event  event;
	struct k_thread *thread;

	k_event_init(&event);

	/*
	 * The type of wait queue used by the event may vary depending upon
	 * which kernel features have been enabled. As such, the most flexible
	 * useful check is to verify that the waitq is empty.
	 */


	thread = z_waitq_head(&event.wait_q);

	zassert_is_null(thread, NULL);
	zassert_true(event.events == 0);
}

static void receive_existing_events(void)
{
	/*
	 * Sync point 1-1 : test_event contains events 0x1234.
	 * Test for events 0x2448 (no waiting)--expect an error
	 */

	k_sem_take(&sync_sem, K_FOREVER);
	test_events = k_event_wait(&test_event, 0x2448, false, K_NO_WAIT);
	k_sem_give(&receiver_sem);

	/*
	 * Sync point 1-2 : test_event still contains event 0x1234.
	 * Test for events 0x2448 (with waiting)--expect an error
	 */

	k_sem_take(&sync_sem, K_FOREVER);
	test_events = k_event_wait(&test_event, 0x2448, false, SHORT_TIMEOUT);
	k_sem_give(&receiver_sem);

	/*
	 * Sync point 1-3: test_event still contains event 0x1234.
	 * Test for events 0x1235 (no waiting)--expect an error
	 */

	k_sem_take(&sync_sem, K_FOREVER);
	test_events = k_event_wait_all(&test_event, 0x1235, false, K_NO_WAIT);
	k_sem_give(&receiver_sem);

	/*
	 * Sync point 1-4: test_event still contains event 0x1234.
	 * Test for events 0x1235 (no waiting)--expect an error
	 */

	k_sem_take(&sync_sem, K_FOREVER);
	test_events = k_event_wait_all(&test_event, 0x1235, false, K_NO_WAIT);
	k_sem_give(&receiver_sem);

	/*
	 * Sync point 1-5: test_event still contains event 0x1234.
	 * Test for events 0x0235. Expect 0x0234 to be returned.
	 */

	k_sem_take(&sync_sem, K_FOREVER);
	test_events = k_event_wait(&test_event, 0x0235, false, K_NO_WAIT);
	k_sem_give(&receiver_sem);

	/*
	 * Sync point 1-6: test_event still contains event 0x1234.
	 * Test for events 0x1234. Expect 0x1234 to be returned.
	 */

	k_sem_take(&sync_sem, K_FOREVER);
	test_events = k_event_wait_all(&test_event, 0x1234, false, K_NO_WAIT);
	k_sem_give(&receiver_sem);
}

static void reset_on_wait(void)
{
	/* Sync point 2-1 */

	k_sem_take(&sync_sem, K_FOREVER);
	test_events = k_event_wait_all(&test_event, 0x1234, true,
				       SHORT_TIMEOUT);
	k_sem_give(&receiver_sem);

	/* Sync point 2-2 */

	k_sem_take(&sync_sem, K_FOREVER);
	test_events = k_event_wait(&test_event, 0x120000, true,
				   SHORT_TIMEOUT);
	k_sem_give(&receiver_sem);

	/* Sync point 2-3 */

	k_sem_take(&sync_sem, K_FOREVER);
	test_events = k_event_wait_all(&test_event, 0x248001, true,
				       SHORT_TIMEOUT);
	k_sem_give(&receiver_sem);

	/* Sync point 2-4 */

	k_sem_take(&sync_sem, K_FOREVER);
	test_events = k_event_wait(&test_event, 0x123458, true,
				   SHORT_TIMEOUT);
	k_sem_give(&receiver_sem);
}

/**
 * receiver helper task
 */

static void receiver(void *p1, void *p2, void *p3)
{
	receive_existing_events();

	reset_on_wait();
}

/**
 * Works with receive_existing_events() to test the waiting for events
 * when some events have already been sent. No additional events are sent
 * to the event object during this block of testing.
 */
static void test_receive_existing_events(void)
{
	int  rv;

	/*
	 * Sync point 1-1.
	 * K_NO_WAIT, k_event_wait(), no matches
	 */

	k_sem_give(&sync_sem);
	rv = k_sem_take(&receiver_sem, LONG_TIMEOUT);
	zassert_true(rv == 0);
	zassert_true(test_events == 0);

	/*
	 * Sync point 1-2.
	 * Short timeout, k_event_wait(), no expected matches
	 */

	k_sem_give(&sync_sem);
	rv = k_sem_take(&receiver_sem, LONG_TIMEOUT);
	zassert_true(rv == 0);
	zassert_true(test_events == 0);

	/*
	 * Sync point 1-3.
	 * K_NO_WAIT, k_event_wait_all(), incomplete match
	 */

	k_sem_give(&sync_sem);
	rv = k_sem_take(&receiver_sem, LONG_TIMEOUT);
	zassert_true(rv == 0);
	zassert_true(test_events == 0);

	/*
	 * Sync point 1-4.
	 * Short timeout, k_event_wait_all(), incomplete match
	 */

	k_sem_give(&sync_sem);
	rv = k_sem_take(&receiver_sem, LONG_TIMEOUT);
	zassert_true(rv == 0);
	zassert_true(test_events == 0);

	/*
	 * Sync point 1-5.
	 * Short timeout, k_event_wait_all(), incomplete match
	 */

	k_sem_give(&sync_sem);
	rv = k_sem_take(&receiver_sem, LONG_TIMEOUT);
	zassert_true(rv == 0);
	zassert_true(test_events == 0x234);

	/*
	 * Sync point 1-6.
	 * Short timeout, k_event_wait_all(), incomplete match
	 */

	k_sem_give(&sync_sem);
	rv = k_sem_take(&receiver_sem, LONG_TIMEOUT);
	zassert_true(rv == 0);
	zassert_true(test_events == 0x1234);
}

/**
 * Works with reset_on_wait() to verify that the events stored in the
 * event object are reset at the appropriate time.
 */

static void test_reset_on_wait(void)
{
	int  rv;

	/*
	 * Sync point 2-1. Reset events before receive.
	 * Short timeout, k_event_wait_all(), incomplete match
	 */

	k_sem_give(&sync_sem);
	k_sleep(DELAY);           /* Give receiver thread time to run */
	k_event_post(&test_event, 0x123);
	rv = k_sem_take(&receiver_sem, LONG_TIMEOUT);
	zassert_true(rv == 0);
	zassert_true(test_events == 0);
	zassert_true(test_event.events == 0x123);

	/*
	 * Sync point 2-2. Reset events before receive.
	 * Short timeout, k_event_wait(), no matches
	 */

	k_sem_give(&sync_sem);
	k_sleep(DELAY);
	k_event_post(&test_event, 0x248);
	rv = k_sem_take(&receiver_sem, LONG_TIMEOUT);
	zassert_true(rv == 0);
	zassert_true(test_events == 0);
	zassert_true(test_event.events == 0x248);

	/*
	 * Sync point 2-3. Reset events before receive.
	 * Short timeout, k_event_wait_all(), complete match
	 */

	k_sem_give(&sync_sem);
	k_sleep(DELAY);
	k_event_post(&test_event, 0x248021);
	rv = k_sem_take(&receiver_sem, LONG_TIMEOUT);
	zassert_true(rv == 0);
	zassert_true(test_events == 0x248001);
	zassert_true(test_event.events  == 0x248021);

	/*
	 * Sync point 2-4. Reset events before receive.
	 * Short timeout, k_event_wait(), partial match
	 */

	k_sem_give(&sync_sem);
	k_sleep(DELAY);
	k_event_post(&test_event, 0x123456);
	rv = k_sem_take(&receiver_sem, LONG_TIMEOUT);
	zassert_true(rv == 0);
	zassert_true(test_events == 0x123450);
	zassert_true(test_event.events  == 0x123456);

	k_event_set(&test_event, 0x0);  /* Reset events */
	k_sem_give(&sync_sem);
}

void test_wake_multiple_threads(void)
{
	uint32_t  events;

	/*
	 * The extra threads are expected to be waiting on <sync_event>
	 * Wake them both up.
	 */

	k_event_set(&sync_event, 0xfff);

	/*
	 * The extra threads send back the events they received. Wait
	 * for all of them.
	 */

	events = k_event_wait_all(&test_event, 0x333, false, SHORT_TIMEOUT);

	zassert_true(events == 0x333);
}

/**
 * @brief Verify event posting and clearing primitives update stored events.
 *
 * @details
 * Exercises the value-manipulation event system calls without any waking or
 * blocking, validating both the resulting set of posted events and the
 * previous value each call returns. Runs as a user-mode test so the event
 * system calls are also exercised from an unprivileged thread.
 *
 * Test steps:
 * - Initialize an event object and confirm it holds no events.
 * - Post events and verify k_event_post() returns the prior set and ORs in
 *   the new bits.
 * - Overwrite events with k_event_set() and verify it returns the prior set.
 * - Use k_event_set_masked() to selectively set/clear bits and verify only
 *   the masked bits change while the returned value reports their prior state.
 *
 * Expected result:
 * - After each call the stored events (read via k_event_test()) match the
 *   expected value, and each call returns the previous state of the affected
 *   bits.
 *
 * @see k_event_post()
 * @see k_event_set()
 * @see k_event_set_masked()
 * @see k_event_test()
 */

ZTEST_USER(events_api, test_event_deliver)
{
	struct k_event *event = &deliver_event;
	uint32_t  events;
	uint32_t  events_mask;
	uint32_t  previous;

	k_event_init(event);

	zassert_equal(k_event_test(event, ~0), 0);

	/*
	 * Verify k_event_post()  and k_event_set() update the
	 * events stored in the event object as expected.
	 */

	events = 0xAAAA;
	previous = k_event_post(event, events);
	zassert_equal(previous, 0x0000);
	zassert_equal(k_event_test(event, ~0), events);

	events |= 0x55555ABC;
	previous = k_event_post(event, events);
	zassert_equal(previous, events & 0xAAAA);
	zassert_equal(k_event_test(event, ~0), events);

	events = 0xAAAA0000;
	previous = k_event_set(event, events);
	zassert_equal(previous, 0xAAAA | 0x55555ABC);
	zassert_equal(k_event_test(event, ~0), events);

	/*
	 * Verify k_event_set_masked() update the events
	 * stored in the event object as expected
	 */
	events = 0x33333333;
	(void)k_event_set(event, events);
	zassert_equal(k_event_test(event, ~0), events);

	events_mask = 0x11111111;
	previous = k_event_set_masked(event, 0, events_mask);
	zassert_equal(previous, 0x11111111);
	zassert_equal(k_event_test(event, ~0), 0x22222222);

	events_mask = 0x22222222;
	previous = k_event_set_masked(event, 0, events_mask);
	zassert_equal(previous, 0x22222222);
	zassert_equal(k_event_test(event, ~0), 0);

	events = 0x22222222;
	events_mask = 0x22222222;
	previous = k_event_set_masked(event, events, events_mask);
	zassert_equal(previous, 0x00000000);
	zassert_equal(k_event_test(event, ~0), events);

	events = 0x11111111;
	events_mask = 0x33333333;
	previous = k_event_set_masked(event, events, events_mask);
	zassert_equal(previous, 0x22222222);
	zassert_equal(k_event_test(event, ~0), events);
}

/**
 * @brief Verify k_event_clear() clears specified events.
 *
 * @details
 * k_event_clear() must reset exactly the requested events tracked by the
 * event object, leave all other events untouched, report the prior state of
 * the requested events, and be a no-op for events that are not currently
 * set. Runs as a user-mode test so the clear system call is also exercised
 * through its user-mode verifier.
 *
 * Test steps:
 * - Verify the statically defined event object starts empty, then set a
 *   known pattern (0x33).
 * - Clear a subset (0x11); verify the prior state of those events is
 *   returned and only the requested events are cleared (0x22 remains).
 * - Clear the same subset again; verify it returns 0 (the events were no
 *   longer set) and nothing changes.
 * - Clear all events; verify the event object ends up empty.
 *
 * Expected result:
 * - Only the requested events are cleared, each call reports the prior
 *   state of the requested events, and clearing unset events changes
 *   nothing.
 *
 * @see k_event_clear()
 * @see k_event_test()
 *
 */
ZTEST_USER(events_api, test_k_event_clear)
{
	struct k_event *event = &clear_event;
	uint32_t previous;

	/* statically defined: initialized (empty) at boot */
	zassert_equal(k_event_test(event, ~0), 0);

	(void)k_event_set(event, 0x33);
	zassert_equal(k_event_test(event, ~0), 0x33);

	/* clear a subset: only the requested events go away; the return
	 * value reports the prior state of the requested events
	 */
	previous = k_event_clear(event, 0x11);
	zassert_equal(previous, 0x11);
	zassert_equal(k_event_test(event, ~0), 0x22);

	/* clearing events that are not set is a no-op and reports them
	 * as not having been set
	 */
	previous = k_event_clear(event, 0x11);
	zassert_equal(previous, 0x00);
	zassert_equal(k_event_test(event, ~0), 0x22);

	/* clear everything */
	previous = k_event_clear(event, ~0);
	zassert_equal(previous, 0x22);
	zassert_equal(k_event_test(event, ~0), 0);
}

/**
 * @brief Verify multi-threaded waiting, reset-on-wait and waking of waiters.
 *
 * @details
 * Drives a receiver thread and two extra waiter threads through a scripted
 * sequence of synchronization points to validate the blocking event APIs end
 * to end: matching all-vs-any conditions, returning only the matching bits,
 * optionally resetting the event before waiting, and waking multiple threads
 * blocked on a single event object at once.
 *
 * Test steps:
 * - Pre-load the event object and start a receiver thread plus two extra
 *   threads waiting with k_event_wait()/k_event_wait_all().
 * - Walk the receiver through waits that should miss, partially match and
 *   fully match the posted events, checking the returned bits each time.
 * - Repeat with the reset flag set and confirm the event is cleared before
 *   the new bits are posted.
 * - Wake both extra threads via k_event_set() and collect the events they
 *   report back.
 *
 * Expected result:
 * - Each wait returns exactly the matching bits (0 when the condition is not
 *   satisfied), reset-on-wait clears prior events, and all woken threads
 *   report the expected events.
 *
 * @see k_event_post()
 * @see k_event_set()
 * @see k_event_wait()
 * @see k_event_wait_all()
 */

ZTEST(events_api, test_event_receive)
{

	/* Create helper threads */

	k_event_set(&test_event, 0x1234);

	(void) k_thread_create(&treceiver, sreceiver, STACK_SIZE,
			       receiver, NULL, NULL, NULL,
			       K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	(void) k_thread_create(&textra1, sextra1, STACK_SIZE,
			       entry_extra1, NULL, NULL, NULL,
			       K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	(void) k_thread_create(&textra2, sextra2, STACK_SIZE,
			       entry_extra2, NULL, NULL, NULL,
			       K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	test_receive_existing_events();

	test_reset_on_wait();

	test_wake_multiple_threads();
}

/**
 * @brief Verify k_event_wait_safe() consumes only the bits it returns.
 *
 * @details
 * The "safe" wait variant atomically clears the events it reports so a caller
 * never observes the same event twice. This test confirms that requested bits
 * present in the object are returned and removed, and that a subsequent wait
 * for the same bits returns nothing. Runs as a user-mode test.
 *
 * Test steps:
 * - Set a known set of events on the object.
 * - Wait (any-match) for a subset and verify exactly that subset is returned.
 * - Wait again for the same subset and verify no events are returned (the bits
 *   were consumed).
 * - Repeat for the remaining bits, including an all-ones request.
 *
 * Expected result:
 * - Each call returns only the matching bits that were present and removes
 *   them, so re-waiting for already-consumed bits yields 0.
 *
 * @see k_event_wait_safe()
 */
ZTEST_USER(events_api, test_k_event_wait_safe)
{
	uint32_t events;

	k_event_set(&test_event, 0x73);

	events = k_event_wait_safe(&test_event, 0x11, false, K_NO_WAIT);
	zexpect_equal(events, 0x11, "expected 0x11, got %x", events);

	events = k_event_wait_safe(&test_event, 0x11, false, K_NO_WAIT);
	zexpect_equal(events, 0x0, "phantom events %x not removed from event object", events);

	events = k_event_wait_safe(&test_event, 0x62, false, K_NO_WAIT);
	zexpect_equal(events, 0x62, "expected 0x62, got %x", events);

	events = k_event_wait_safe(&test_event, -1, false, K_NO_WAIT);
	zexpect_equal(events, 0x0, "phantom events %x not removed from event object", events);
}

/**
 * @brief Verify k_event_wait_all_safe() consumes bits only on a full match.
 *
 * @details
 * The all-match "safe" wait variant returns and clears the requested bits only
 * when every requested bit is present; a partial match returns nothing and
 * leaves the object untouched. This test exercises both the no-match and the
 * complete-match cases. Runs as a user-mode test.
 *
 * Test steps:
 * - Set a known set of events on the object.
 * - Wait-all for a set that is not fully present and verify 0 is returned.
 * - Wait-all for a fully-present subset and verify exactly that subset is
 *   returned and consumed.
 * - Repeat for the remaining bits.
 *
 * Expected result:
 * - A partial match returns 0 and consumes nothing; a complete match returns
 *   the requested bits and removes them from the object.
 *
 * @see k_event_wait_all_safe()
 */
ZTEST_USER(events_api, test_k_event_wait_all_safe)
{
	uint32_t events;

	k_event_set(&test_event, 0x73);

	events = k_event_wait_all_safe(&test_event, 0x81, false, K_NO_WAIT);
	zexpect_equal(events, 0x0, "expected 0x0, got %x", events);

	events = k_event_wait_all_safe(&test_event, 0x11, false, K_NO_WAIT);
	zexpect_equal(events, 0x11, "expected 0x11, got %x", events);

	events = k_event_wait_all_safe(&test_event, 0x63, false, K_NO_WAIT);
	zexpect_equal(events, 0x0, "expected 0x0, got %x", events);

	events = k_event_wait_all_safe(&test_event, 0x62, false, K_NO_WAIT);
	zexpect_equal(events, 0x62, "expected 0x62, got %x", events);
}

/**
 * Setup routine for the events test suite.
 *
 * Grant the test thread access to the event objects exercised by the
 * user-mode (ZTEST_USER) test cases so they can be reached through the
 * event system calls when CONFIG_USERSPACE is enabled.
 */
static void *events_api_setup(void)
{
	k_thread_access_grant(k_current_get(), &test_event, &deliver_event,
			       &clear_event);

	return NULL;
}

/**
 * @}
 */

ZTEST_SUITE(events_api, NULL, events_api_setup,
		ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
