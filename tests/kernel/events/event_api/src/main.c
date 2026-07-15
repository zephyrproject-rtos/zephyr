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
static K_EVENT_DEFINE(deliver_event);
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
 * receiver helper task entry points, one per test scenario
 */

static void receiver_existing(void *p1, void *p2, void *p3)
{
	receive_existing_events();
}

static void receiver_reset(void *p1, void *p2, void *p3)
{
	reset_on_wait();
}

/**
 * Works with receive_existing_events() to test the waiting for events
 * when some events have already been sent. No additional events are sent
 * to the event object during this block of testing.
 */
static void drive_receive_existing_events(void)
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

static void drive_reset_on_wait(void)
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
}

/**
 * @brief Verify k_event_post() ORs new events into the stored set.
 *
 * @details
 * Exercises the posting system call without any waking or blocking,
 * validating both the resulting set of posted events and the previous
 * value each call returns. Runs as a user-mode test so the post system
 * call is also exercised from an unprivileged thread.
 *
 * Test steps:
 * - Empty the event object and confirm it holds no events.
 * - Post a first set of events and verify it is stored verbatim and that
 *   the call reports no prior events.
 * - Post an overlapping second set and verify the new bits are ORed into
 *   the stored set while the call returns the prior state of the posted
 *   bits.
 *
 * Expected result:
 * - After each call the stored events (read via k_event_test()) are the
 *   union of everything posted so far, and each call returns the previous
 *   state of the affected bits.
 *
 * @see k_event_post()
 * @see k_event_test()
 */
ZTEST_USER(events_api, test_event_post)
{
	struct k_event *event = &deliver_event;
	uint32_t  events;
	uint32_t  previous;

	(void)k_event_set(event, 0);

	zassert_equal(k_event_test(event, ~0), 0);

	events = 0xAAAA;
	previous = k_event_post(event, events);
	zassert_equal(previous, 0x0000);
	zassert_equal(k_event_test(event, ~0), events);

	events |= 0x55555ABC;
	previous = k_event_post(event, events);
	zassert_equal(previous, events & 0xAAAA);
	zassert_equal(k_event_test(event, ~0), events);
}

/**
 * @brief Verify k_event_set() overwrites the stored set of events.
 *
 * @details
 * Exercises the set system call without any waking or blocking, validating
 * that the stored events are replaced (not merged) and that the previous
 * set is returned. Runs as a user-mode test so the set system call is also
 * exercised from an unprivileged thread.
 *
 * Test steps:
 * - Empty the event object and post a known set of events.
 * - Overwrite the events with k_event_set() and verify the stored set is
 *   exactly the new value while the call returns the prior set.
 * - Set a second value and verify the first one is reported back.
 *
 * Expected result:
 * - After each call the stored events (read via k_event_test()) match the
 *   set value exactly, and each call returns the complete previous set.
 *
 * @see k_event_set()
 * @see k_event_test()
 */
ZTEST_USER(events_api, test_event_set)
{
	struct k_event *event = &deliver_event;
	uint32_t  previous;

	(void)k_event_set(event, 0);

	(void)k_event_post(event, 0xAAAA | 0x55555ABC);

	previous = k_event_set(event, 0xAAAA0000);
	zassert_equal(previous, 0xAAAA | 0x55555ABC);
	zassert_equal(k_event_test(event, ~0), 0xAAAA0000);

	previous = k_event_set(event, 0x33333333);
	zassert_equal(previous, 0xAAAA0000);
	zassert_equal(k_event_test(event, ~0), 0x33333333);
}

/**
 * @brief Verify k_event_set_masked() changes only the masked events.
 *
 * @details
 * Exercises the masked-set system call without any waking or blocking,
 * validating that only the events selected by the mask are set or cleared,
 * that all other events are untouched, and that the previous state of the
 * masked events is returned. Runs as a user-mode test so the masked-set
 * system call is also exercised from an unprivileged thread.
 *
 * Test steps:
 * - Set a known pattern of events (0x33333333).
 * - Clear one masked subset at a time and verify only those bits drop out
 *   while their prior state is returned.
 * - Set masked subsets and verify only the masked bits change, including a
 *   mask that simultaneously sets some bits and clears others.
 *
 * Expected result:
 * - After each call the stored events (read via k_event_test()) differ from
 *   the prior state only in the masked bits, and each call returns the
 *   previous state of the masked bits.
 *
 * @see k_event_set_masked()
 * @see k_event_test()
 */
ZTEST_USER(events_api, test_event_set_masked)
{
	struct k_event *event = &deliver_event;
	uint32_t  events;
	uint32_t  events_mask;
	uint32_t  previous;

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
 * @brief Verify any/all matching of a wait against already-posted events.
 *
 * @details
 * Drives a receiver thread through a scripted sequence of synchronization
 * points to validate the matching rules of the blocking event APIs against
 * a pre-loaded event object: an any-match wait completes when at least one
 * requested event is present, an all-match wait completes only when every
 * requested event is present, and a completed wait reports exactly the
 * matching events. No events are posted while the receiver is waiting.
 *
 * Test steps:
 * - Pre-load the event object with a known set of events (0x1234) and start
 *   the receiver thread.
 * - Walk the receiver through k_event_wait() and k_event_wait_all() calls
 *   that should miss (no overlap, incomplete overlap) and verify each
 *   reports no events.
 * - Walk the receiver through a partially matching k_event_wait() and a
 *   fully matching k_event_wait_all() and verify the returned bits.
 *
 * Expected result:
 * - Waits whose condition is not met return 0, a matching any-wait returns
 *   only the overlapping bits (0x0234), and a matching all-wait returns
 *   exactly the requested bits (0x1234).
 *
 * @see k_event_wait()
 * @see k_event_wait_all()
 */
ZTEST(events_api, test_event_receive_existing)
{
	k_sem_init(&sync_sem, 0, 1);
	k_sem_init(&receiver_sem, 0, 1);
	k_event_set(&test_event, 0x1234);

	(void) k_thread_create(&treceiver, sreceiver, STACK_SIZE,
			       receiver_existing, NULL, NULL, NULL,
			       K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	drive_receive_existing_events();

	zassert_equal(k_thread_join(&treceiver, LONG_TIMEOUT), 0);
}

/**
 * @brief Verify waiting for events accepts and honors a timeout.
 *
 * @details
 * Focused test for the timeout behavior of the blocking event APIs: a wait
 * whose condition is not satisfied within the given timeout returns no
 * events, and only after the timeout has elapsed. Runs as a user-mode test
 * so the wait system calls are also exercised from an unprivileged thread.
 *
 * Test steps:
 * - Empty the event object.
 * - Wait for events with K_NO_WAIT and verify the immediate no-event return.
 * - Wait for events with a finite timeout and verify no events are returned
 *   and at least the timeout duration elapsed.
 * - Post a partial match and verify an all-match wait still times out.
 *
 * Expected result:
 * - Each unsatisfied wait returns 0, the timed waits only return once their
 *   timeout has expired.
 *
 * @see k_event_wait()
 * @see k_event_wait_all()
 */
ZTEST_USER(events_api, test_event_wait_timeout)
{
	uint32_t  events;
	int64_t   reftime;
	int64_t   elapsed;

	k_event_set(&test_event, 0);

	events = k_event_wait(&test_event, 0xFFF, false, K_NO_WAIT);
	zassert_equal(events, 0);

	reftime = k_uptime_get();
	events = k_event_wait(&test_event, 0xFFF, false, SHORT_TIMEOUT);
	elapsed = k_uptime_delta(&reftime);
	zassert_equal(events, 0);
	zassert_true(elapsed >= 100, "wait returned after %d ms", (int)elapsed);

	/* A partial match must not satisfy an all-match wait */
	k_event_set(&test_event, 0x00F);
	reftime = k_uptime_get();
	events = k_event_wait_all(&test_event, 0xFFF, false, SHORT_TIMEOUT);
	elapsed = k_uptime_delta(&reftime);
	zassert_equal(events, 0);
	zassert_true(elapsed >= 100, "wait returned after %d ms", (int)elapsed);

	k_event_set(&test_event, 0);
}

/**
 * @brief Verify the reset option clears prior events before waiting.
 *
 * @details
 * Drives a receiver thread through waits issued with the reset flag set,
 * posting fresh events after each wait has started. Because the event
 * object is cleared when the wait begins, only the newly posted events can
 * satisfy the wait condition, and threads whose condition is met by the
 * post are woken while others time out.
 *
 * Test steps:
 * - Pre-load the event object and start the receiver thread.
 * - Have the receiver wait with reset=true while events that do not satisfy
 *   the condition are posted; verify the wait times out, the pre-existing
 *   events were discarded and only the new post remains stored.
 * - Repeat with posts that fully or partially satisfy the condition and
 *   verify the receiver is woken with exactly the matching events.
 *
 * Expected result:
 * - Each wait observes only events posted after it started (prior events
 *   are reset), unsatisfied waits time out, and satisfied waits wake the
 *   receiver and return the matching events.
 *
 * @see k_event_post()
 * @see k_event_wait()
 * @see k_event_wait_all()
 */
ZTEST(events_api, test_event_reset_on_wait)
{
	k_sem_init(&sync_sem, 0, 1);
	k_sem_init(&receiver_sem, 0, 1);
	k_event_set(&test_event, 0x1234);

	(void) k_thread_create(&treceiver, sreceiver, STACK_SIZE,
			       receiver_reset, NULL, NULL, NULL,
			       K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	drive_reset_on_wait();

	zassert_equal(k_thread_join(&treceiver, LONG_TIMEOUT), 0);
}

/**
 * @brief Verify a single event delivery wakes all matching waiters.
 *
 * @details
 * Parks two threads on the same event object -- one waiting for any of its
 * events, one waiting for all of them -- and wakes both with a single
 * k_event_set() call. Each woken thread posts the events it received back
 * to a second event object so the test can confirm both were unpended with
 * their expected events.
 *
 * Test steps:
 * - Empty both event objects and start the two waiter threads.
 * - Give the waiters time to pend on the sync event object.
 * - Deliver events matching both wait conditions with one k_event_set().
 * - Collect the events each waiter reports back and join the waiters.
 *
 * Expected result:
 * - Both waiting threads are woken by the single delivery and report
 *   exactly the events that matched their respective wait conditions.
 *
 * @see k_event_set()
 * @see k_event_wait()
 * @see k_event_wait_all()
 */
ZTEST(events_api, test_event_wake_multiple)
{
	uint32_t  events;

	k_event_set(&test_event, 0);
	k_event_set(&sync_event, 0);

	(void) k_thread_create(&textra1, sextra1, STACK_SIZE,
			       entry_extra1, NULL, NULL, NULL,
			       K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	(void) k_thread_create(&textra2, sextra2, STACK_SIZE,
			       entry_extra2, NULL, NULL, NULL,
			       K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	/* Let both waiters pend on <sync_event>, then wake them together. */
	k_sleep(DELAY);
	k_event_set(&sync_event, 0xfff);

	/*
	 * The extra threads send back the events they received. Wait
	 * for all of them.
	 */
	events = k_event_wait_all(&test_event, 0x333, false, SHORT_TIMEOUT);
	zassert_true(events == 0x333);

	zassert_equal(k_thread_join(&textra1, LONG_TIMEOUT), 0);
	zassert_equal(k_thread_join(&textra2, LONG_TIMEOUT), 0);
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
