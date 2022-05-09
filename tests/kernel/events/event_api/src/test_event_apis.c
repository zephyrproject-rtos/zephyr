/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/wait_q.h>

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

static K_SEM_DEFINE(receiver_sem, 0, 1);
static K_SEM_DEFINE(sync_sem, 0, 1);

volatile static uint32_t test_events;

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
 * Test the k_event_init() API.
 *
 * This is a white-box test to verify that the k_event_init() API initializes
 * the fields of a k_event structure as expected.
 */

void test_k_event_init(void)
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
	zassert_true(event.events == 0, NULL);
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
	zassert_true(rv == 0, NULL);
	zassert_true(test_events == 0, NULL);

	/*
	 * Sync point 1-2.
	 * Short timeout, k_event_wait(), no expected matches
	 */

	k_sem_give(&sync_sem);
	rv = k_sem_take(&receiver_sem, LONG_TIMEOUT);
	zassert_true(rv == 0, NULL);
	zassert_true(test_events == 0, NULL);

	/*
	 * Sync point 1-3.
	 * K_NO_WAIT, k_event_wait_all(), incomplete match
	 */

	k_sem_give(&sync_sem);
	rv = k_sem_take(&receiver_sem, LONG_TIMEOUT);
	zassert_true(rv == 0, NULL);
	zassert_true(test_events == 0, NULL);

	/*
	 * Sync point 1-4.
	 * Short timeout, k_event_wait_all(), incomplete match
	 */

	k_sem_give(&sync_sem);
	rv = k_sem_take(&receiver_sem, LONG_TIMEOUT);
	zassert_true(rv == 0, NULL);
	zassert_true(test_events == 0, NULL);

	/*
	 * Sync point 1-5.
	 * Short timeout, k_event_wait_all(), incomplete match
	 */

	k_sem_give(&sync_sem);
	rv = k_sem_take(&receiver_sem, LONG_TIMEOUT);
	zassert_true(rv == 0, NULL);
	zassert_true(test_events == 0x234, NULL);

	/*
	 * Sync point 1-6.
	 * Short timeout, k_event_wait_all(), incomplete match
	 */

	k_sem_give(&sync_sem);
	rv = k_sem_take(&receiver_sem, LONG_TIMEOUT);
	zassert_true(rv == 0, NULL);
	zassert_true(test_events == 0x1234, NULL);
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
	zassert_true(rv == 0, NULL);
	zassert_true(test_events == 0, NULL);
	zassert_true(test_event.events == 0x123, NULL);

	/*
	 * Sync point 2-2. Reset events before receive.
	 * Short timeout, k_event_wait(), no matches
	 */

	k_sem_give(&sync_sem);
	k_sleep(DELAY);
	k_event_post(&test_event, 0x248);
	rv = k_sem_take(&receiver_sem, LONG_TIMEOUT);
	zassert_true(rv == 0, NULL);
	zassert_true(test_events == 0, NULL);
	zassert_true(test_event.events == 0x248, NULL);

	/*
	 * Sync point 2-3. Reset events before receive.
	 * Short timeout, k_event_wait_all(), complete match
	 */

	k_sem_give(&sync_sem);
	k_sleep(DELAY);
	k_event_post(&test_event, 0x248021);
	rv = k_sem_take(&receiver_sem, LONG_TIMEOUT);
	zassert_true(rv == 0, NULL);
	zassert_true(test_events == 0x248001, NULL);
	zassert_true(test_event.events  == 0x248021, NULL);

	/*
	 * Sync point 2-4. Reset events before receive.
	 * Short timeout, k_event_wait(), partial match
	 */

	k_sem_give(&sync_sem);
	k_sleep(DELAY);
	k_event_post(&test_event, 0x123456);
	rv = k_sem_take(&receiver_sem, LONG_TIMEOUT);
	zassert_true(rv == 0, NULL);
	zassert_true(test_events == 0x123450, NULL);
	zassert_true(test_event.events  == 0x123456, NULL);

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

	zassert_true(events == 0x333, NULL);
}

/**
 * Test basic k_event_post() and k_event_set() APIs.
 *
 * Tests the basic k_event_post() and k_event_set() APIs. This does not
 * involve waking or receiving events.
 */

void test_event_deliver(void)
{
	static struct k_event  event;
	uint32_t  events;

	k_event_init(&event);

	zassert_true(event.events == 0, NULL);

	/*
	 * Verify k_event_post()  and k_event_set() update the
	 * events stored in the event object as expected.
	 */

	events = 0xAAAA;
	k_event_post(&event, events);
	zassert_true(event.events == events, NULL);

	events |= 0x55555ABC;
	k_event_post(&event, events);
	zassert_true(event.events == events, NULL);

	events = 0xAAAA0000;
	k_event_set(&event, events);
	zassert_true(event.events == events, NULL);
}

/**
 * Test delivery and reception of events.
 *
 * Testing both the delivery and reception of events involves the use of
 * multiple threads and uses the following event related APIs:
 *   k_event_post(), k_event_set(), k_event_wait() and k_event_wait_all().
 */

void test_event_receive(void)
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
