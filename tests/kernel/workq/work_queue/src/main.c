/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

/* This test covers deprecated API.  Avoid inappropriate diagnostics
 * about the use of that API.
 */
#include <zephyr/toolchain.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/tc_util.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

#define NUM_TEST_ITEMS          6

/* Each work item takes 100ms by default. */
#define WORK_ITEM_WAIT (CONFIG_TEST_WORK_ITEM_WAIT_MS)

/* In fact, each work item could take up to this value */
#define WORK_ITEM_WAIT_ALIGNED	\
	k_ticks_to_ms_floor64(k_ms_to_ticks_ceil32(WORK_ITEM_WAIT) + _TICK_ALIGN)

/*
 * Wait 50ms between work submissions, to ensure co-op and prempt
 * preempt thread submit alternatively.
 */
#define SUBMIT_WAIT	(CONFIG_TEST_SUBMIT_WAIT_MS)

#define STACK_SIZE      (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

/* How long to wait for the full test suite to complete.  Allow for a
 * little slop
 */
#define CHECK_WAIT ((NUM_TEST_ITEMS + 1) * WORK_ITEM_WAIT_ALIGNED)

struct delayed_test_item {
	int key;
	struct k_work_delayable work;
};

struct triggered_test_item {
	int key;
	struct k_work_poll work;
	struct k_poll_signal signal;
	struct k_poll_event event;
};


static K_THREAD_STACK_DEFINE(co_op_stack, STACK_SIZE);
static struct k_thread co_op_data;

static struct delayed_test_item delayed_tests[NUM_TEST_ITEMS];
static struct triggered_test_item triggered_tests[NUM_TEST_ITEMS];

static int results[NUM_TEST_ITEMS];
static int num_results;
static int expected_poll_result;

#define MSG_PROVIDER_THREAD_STACK_SIZE 2048
#define MSG_CONSUMER_WORKQ_STACK_SIZE 2048

#define MSG_PROVIDER_THREAD_PRIO K_PRIO_PREEMPT(8)
#define MSG_CONSUMER_WORKQ_PRIO K_PRIO_COOP(7)
#define MSG_SIZE 16U

static K_THREAD_STACK_DEFINE(provider_thread_stack, MSG_PROVIDER_THREAD_STACK_SIZE);
static K_THREAD_STACK_DEFINE(consumer_workq_stack, MSG_CONSUMER_WORKQ_STACK_SIZE);

struct triggered_from_msgq_test_item {
	k_tid_t tid;
	struct k_thread msg_provider_thread;
	struct k_work_q msg_consumer_workq;
	struct k_work_poll work;
	char msgq_buf[1][MSG_SIZE];
	struct k_msgq msgq;
	struct k_poll_event event;
};

static struct triggered_from_msgq_test_item triggered_from_msgq_test;

static void work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct delayed_test_item *ti =
			CONTAINER_OF(dwork, struct delayed_test_item, work);

	LOG_DBG(" - Running test item %d", ti->key);
	k_msleep(WORK_ITEM_WAIT);

	results[num_results++] = ti->key;
}

/**
 * @ingroup kernel_workqueue_tests
 * @see k_work_init()
 */
static void delayed_test_items_init(void)
{
	int i;

	for (i = 0; i < NUM_TEST_ITEMS; i++) {
		delayed_tests[i].key = i + 1;
		k_work_init_delayable(&delayed_tests[i].work, work_handler);
	}
}

static void reset_results(void)
{
	int i;

	for (i = 0; i < NUM_TEST_ITEMS; i++) {
		results[i] = 0;
	}

	num_results = 0;
}

static void coop_work_main(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int i;

	/* Let the preempt thread submit the first work item. */
	k_msleep(SUBMIT_WAIT / 2);

	for (i = 1; i < NUM_TEST_ITEMS; i += 2) {
		LOG_DBG(" - Submitting work %d from coop thread", i + 1);
		k_work_schedule(&delayed_tests[i].work, K_NO_WAIT);
		k_msleep(SUBMIT_WAIT);
	}
}

/**
 * @ingroup kernel_workqueue_tests
 * @see k_work_submit()
 */
static void delayed_test_items_submit(void)
{
	int i;

	k_thread_create(&co_op_data, co_op_stack, STACK_SIZE,
			coop_work_main,
			NULL, NULL, NULL, K_PRIO_COOP(10), 0, K_NO_WAIT);

	for (i = 0; i < NUM_TEST_ITEMS; i += 2) {
		LOG_DBG(" - Submitting work %d from preempt thread", i + 1);
		k_work_schedule(&delayed_tests[i].work, K_NO_WAIT);
		k_msleep(SUBMIT_WAIT);
	}
}

static void check_results(int num_tests)
{
	int i;

	zassert_equal(num_results, num_tests,
		      "*** work items finished: %d (expected: %d)\n",
		      num_results, num_tests);

	for (i = 0; i < num_tests; i++) {
		zassert_equal(results[i], i + 1,
			      "*** got result %d in position %d"
			      " (expected %d)\n",
			      results[i], i, i + 1);
	}

}

/**
 * @brief Test work queue items submission sequence
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_init(), k_work_submit()
 */
static void test_sequence(void)
{
	LOG_DBG(" - Initializing test items");
	delayed_test_items_init();

	LOG_DBG(" - Submitting test items");
	delayed_test_items_submit();

	LOG_DBG(" - Waiting for work to finish");
	k_msleep(CHECK_WAIT);

	check_results(NUM_TEST_ITEMS);
	reset_results();
}



static void resubmit_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct delayed_test_item *ti =
			CONTAINER_OF(dwork, struct delayed_test_item, work);

	k_msleep(WORK_ITEM_WAIT);

	results[num_results++] = ti->key;

	if (ti->key < NUM_TEST_ITEMS) {
		ti->key++;
		LOG_DBG(" - Resubmitting work");
		k_work_submit(work);
	}
}
/**
 * @brief Verify a work item handler can resubmit its own work item.
 *
 * @details
 * A work item handler resubmits the same work item (with an incremented key)
 * each time it runs, until a target count is reached. This confirms the work
 * queue thread invokes the handler for every submission and that a completed
 * work item may be submitted again from within its own handler.
 *
 * Test steps:
 * - Initialize a work item whose handler resubmits itself until the target
 *   count is reached.
 * - Submit the work item once and wait for all iterations to complete.
 * - Verify the handler ran once per iteration in sequence.
 *
 * Expected result:
 * - The handler is invoked for every (re)submission and all results are
 *   recorded in order.
 *
 * @see k_work_submit(), k_work_schedule()
 * @verifies ZEP-SRS-26-13
 */
ZTEST(workqueue_triggered, test_resubmit)
{
	LOG_DBG("Starting resubmit test");

	delayed_tests[0].key = 1;
	k_work_init_delayable(&delayed_tests[0].work, resubmit_work_handler);

	LOG_DBG(" - Submitting work");
	k_work_schedule(&delayed_tests[0].work, K_NO_WAIT);

	LOG_DBG(" - Waiting for work to finish");
	k_msleep(CHECK_WAIT);

	LOG_DBG(" - Checking results");
	check_results(NUM_TEST_ITEMS);
	reset_results();
}

static void delayed_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct delayed_test_item *ti =
			CONTAINER_OF(dwork, struct delayed_test_item, work);

	LOG_DBG(" - Running delayed test item %d", ti->key);

	results[num_results++] = ti->key;
}

/**
 * @brief Test delayed work queue init
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_init_delayable()
 */
static void test_delayed_init(void)
{
	int i;

	for (i = 0; i < NUM_TEST_ITEMS; i++) {
		delayed_tests[i].key = i + 1;
		k_work_init_delayable(&delayed_tests[i].work,
				      delayed_work_handler);
	}
}

static void coop_delayed_work_main(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int i;

	/* Let the preempt thread submit the first work item. */
	k_msleep(SUBMIT_WAIT / 2);

	for (i = 1; i < NUM_TEST_ITEMS; i += 2) {
		LOG_DBG(" - Submitting delayed work %d from"
			" coop thread", i + 1);
		k_work_schedule(&delayed_tests[i].work,
				K_MSEC((i + 1) * WORK_ITEM_WAIT));
	}
}

/**
 * @brief Test delayed workqueue submit
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_init_delayable(), k_work_schedule()
 */
static void test_delayed_submit(void)
{
	int i;

	k_thread_create(&co_op_data, co_op_stack, STACK_SIZE,
			coop_delayed_work_main,
			NULL, NULL, NULL, K_PRIO_COOP(10), 0, K_NO_WAIT);

	for (i = 0; i < NUM_TEST_ITEMS; i += 2) {
		LOG_DBG(" - Submitting delayed work %d from"
			" preempt thread", i + 1);
		zassert_true(k_work_reschedule(&delayed_tests[i].work,
			     K_MSEC((i + 1) * WORK_ITEM_WAIT)) >= 0, NULL);
	}

}

static void coop_delayed_work_cancel_main(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_work_schedule(&delayed_tests[1].work, K_MSEC(WORK_ITEM_WAIT));

	LOG_DBG(" - Cancel delayed work from coop thread");
	k_work_cancel_delayable(&delayed_tests[1].work);
}

/**
 * @brief Verify a scheduled delayable work item can be cancelled before it is
 * submitted.
 *
 * @details
 * A delayable work item is scheduled with a future delay and then cancelled
 * (from both a preempt and a co-op thread) before the delay elapses. Because
 * the item never reaches its work queue, its handler must not run.
 *
 * Test steps:
 * - Schedule a delayable work item with a future delay.
 * - Cancel it with k_work_cancel_delayable() before the delay elapses.
 * - Repeat the schedule/cancel from a co-op thread.
 * - Wait past the delay and confirm no handler ran.
 *
 * Expected result:
 * - The handler is never invoked (zero results recorded).
 *
 * @see k_work_schedule(), k_work_cancel_delayable()
 * @verifies ZEP-SRS-26-24
 */
ZTEST(workqueue_delayed, test_delayed_cancel)
{
	LOG_DBG("Starting delayed cancel test");

	k_work_schedule(&delayed_tests[0].work, K_MSEC(WORK_ITEM_WAIT));

	LOG_DBG(" - Cancel delayed work from preempt thread");
	k_work_cancel_delayable(&delayed_tests[0].work);

	k_thread_create(&co_op_data, co_op_stack, STACK_SIZE,
			coop_delayed_work_cancel_main,
			NULL, NULL, NULL, K_HIGHEST_THREAD_PRIO, 0, K_NO_WAIT);

	LOG_DBG(" - Waiting for work to finish");
	k_msleep(WORK_ITEM_WAIT_ALIGNED);

	LOG_DBG(" - Checking results");
	check_results(0);
	reset_results();
}

/**
 * @brief Verify the pending status of a delayable work item.
 *
 * @details
 * k_work_delayable_is_pending() reports whether a delayable work item is queued
 * or waiting for its delay to elapse. This test confirms an uninitialized item
 * is not pending, that scheduling it (immediately and with a delay) marks it
 * pending, and that it stops being pending once processed.
 *
 * Test steps:
 * - Initialize a delayable work item and confirm it is not pending.
 * - Schedule it with K_NO_WAIT and confirm it becomes pending then clears once
 *   processed.
 * - Schedule it with a delay and confirm it is pending until the delay elapses.
 *
 * Expected result:
 * - k_work_delayable_is_pending() reflects the item's queued/waiting state at
 *   each step.
 *
 * @see k_work_delayable_is_pending(), k_work_schedule()
 * @verifies ZEP-SRS-26-16
 */
ZTEST(workqueue_delayed, test_delayed_pending)
{
	LOG_DBG("Starting delayed pending test");

	k_work_init_delayable(&delayed_tests[0].work, delayed_work_handler);

	zassert_false(k_work_delayable_is_pending(&delayed_tests[0].work));

	LOG_DBG(" - Check pending delayed work when in workqueue");
	k_work_schedule(&delayed_tests[0].work, K_NO_WAIT);
	zassert_true(k_work_delayable_is_pending(&delayed_tests[0].work));

	k_msleep(1);
	zassert_false(k_work_delayable_is_pending(&delayed_tests[0].work));

	LOG_DBG(" - Checking results");
	check_results(1);
	reset_results();

	LOG_DBG(" - Check pending delayed work with timeout");
	k_work_schedule(&delayed_tests[0].work, K_MSEC(WORK_ITEM_WAIT));
	zassert_true(k_work_delayable_is_pending(&delayed_tests[0].work));

	k_msleep(WORK_ITEM_WAIT_ALIGNED);
	zassert_false(k_work_delayable_is_pending(&delayed_tests[0].work));

	LOG_DBG(" - Checking results");
	check_results(1);
	reset_results();
}

/**
 * @brief Verify delayable work items are submitted to their queue when their
 * delay elapses.
 *
 * @details
 * Multiple delayable work items are scheduled (and rescheduled) from preempt
 * and co-op threads with increasing delays. Each item must be submitted to its
 * work queue when its delay elapses and then have its handler invoked, so that
 * every item completes.
 *
 * Test steps:
 * - Initialize the delayable work items.
 * - Schedule and reschedule them with staggered, increasing delays from two
 *   threads.
 * - Wait for all delays to elapse.
 * - Verify every item's handler ran.
 *
 * Expected result:
 * - All delayable work items are submitted on delay expiry and processed.
 *
 * @see k_work_schedule(), k_work_reschedule()
 * @verifies ZEP-SRS-26-21
 * @verifies ZEP-SRS-26-22
 * @verifies ZEP-SRS-26-23
 */
ZTEST(workqueue_delayed, test_delayed)
{
	LOG_DBG("Starting delayed test");

	LOG_DBG(" - Initializing delayed test items");
	test_delayed_init();

	LOG_DBG(" - Submitting delayed test items");
	test_delayed_submit();

	LOG_DBG(" - Waiting for delayed work to finish");
	k_msleep(CHECK_WAIT);

	LOG_DBG(" - Checking results");
	check_results(NUM_TEST_ITEMS);
	reset_results();
}

static void triggered_work_handler(struct k_work *work)
{
	struct k_work_poll *pwork = CONTAINER_OF(work, struct k_work_poll, work);
	struct triggered_test_item *ti =
			CONTAINER_OF(pwork, struct triggered_test_item, work);

	LOG_DBG(" - Running triggered test item %d", ti->key);

	zassert_equal(ti->work.poll_result, expected_poll_result,
		     "res %d expect %d", ti->work.poll_result, expected_poll_result);

	results[num_results++] = ti->key;
}

/**
 * @brief Test triggered work queue init
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_poll_init()
 */
static void test_triggered_init(void)
{
	int i;

	for (i = 0; i < NUM_TEST_ITEMS; i++) {
		triggered_tests[i].key = i + 1;
		k_work_poll_init(&triggered_tests[i].work,
				 triggered_work_handler);

		k_poll_signal_init(&triggered_tests[i].signal);
		k_poll_event_init(&triggered_tests[i].event,
				  K_POLL_TYPE_SIGNAL,
				  K_POLL_MODE_NOTIFY_ONLY,
				  &triggered_tests[i].signal);
	}
}

/**
 * @brief Test triggered workqueue submit
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_poll_init(), k_work_poll_submit()
 */
static void test_triggered_submit(k_timeout_t timeout)
{
	int i;

	for (i = 0; i < NUM_TEST_ITEMS; i++) {
		LOG_DBG(" - Submitting triggered work %d", i + 1);
		zassert_true(k_work_poll_submit(&triggered_tests[i].work,
						&triggered_tests[i].event,
						1, timeout) == 0, NULL);
	}
}

/**
 * @brief Trigger triggered workqueue execution
 *
 * @ingroup kernel_workqueue_tests
 */
static void test_triggered_trigger(void)
{
	int i;

	for (i = 0; i < NUM_TEST_ITEMS; i++) {
		LOG_DBG(" - Triggering work %d", i + 1);
		zassert_true(k_poll_signal_raise(&triggered_tests[i].signal,
						 1) == 0, NULL);
	}
}

/**
 * @brief Verify triggered (poll) work items run once their event is signalled.
 *
 * @details
 * Triggered work items are submitted against poll events and only run once the
 * event becomes ready. This test submits the items, then raises their signals,
 * and confirms each handler runs and observes a successful poll result.
 *
 * Test steps:
 * - Initialize the triggered work items and their poll signals/events.
 * - Submit the items with k_work_poll_submit() (K_FOREVER timeout).
 * - Raise each signal and wait for the handlers to run.
 *
 * Expected result:
 * - Every triggered handler runs with a poll result of 0.
 *
 * @see k_work_poll_init(), k_work_poll_submit()
 */
ZTEST(workqueue_triggered, test_triggered)
{
	LOG_DBG("Starting triggered test");

	/* As work items are triggered, they should indicate an event. */
	expected_poll_result = 0;

	LOG_DBG(" - Initializing triggered test items");
	test_triggered_init();

	LOG_DBG(" - Submitting triggered test items");
	test_triggered_submit(K_FOREVER);

	LOG_DBG(" - Triggering test items execution");
	test_triggered_trigger();

	/* Items should be executed when we will be sleeping. */
	k_msleep(WORK_ITEM_WAIT);

	LOG_DBG(" - Checking results");
	check_results(NUM_TEST_ITEMS);
	reset_results();
}

/**
 * @brief Verify triggered (poll) work items run when their event is already
 * signalled at submission.
 *
 * @details
 * If the poll event of a triggered work item is already signalled before the
 * item is submitted, the item must still run. This test raises the signals
 * first and submits the items afterwards.
 *
 * Test steps:
 * - Initialize the triggered work items and their poll signals/events.
 * - Raise each signal before submitting.
 * - Submit the items with k_work_poll_submit() and wait for them to run.
 *
 * Expected result:
 * - Every triggered handler runs with a poll result of 0.
 *
 * @see k_work_poll_init(), k_work_poll_submit()
 */
ZTEST(workqueue_triggered, test_already_triggered)
{
	LOG_DBG("Starting triggered test");

	/* As work items are triggered, they should indicate an event. */
	expected_poll_result = 0;

	LOG_DBG(" - Initializing triggered test items");
	test_triggered_init();

	LOG_DBG(" - Triggering test items execution");
	test_triggered_trigger();

	LOG_DBG(" - Submitting triggered test items");
	test_triggered_submit(K_FOREVER);

	/* Items should be executed when we will be sleeping. */
	k_msleep(WORK_ITEM_WAIT);

	LOG_DBG(" - Checking results");
	check_results(NUM_TEST_ITEMS);
	reset_results();
}

static void triggered_resubmit_work_handler(struct k_work *work)
{
	struct k_work_poll *pwork = CONTAINER_OF(work, struct k_work_poll, work);
	struct triggered_test_item *ti =
			CONTAINER_OF(pwork, struct triggered_test_item, work);

	results[num_results++] = ti->key;

	if (ti->key < NUM_TEST_ITEMS) {
		ti->key++;
		LOG_DBG(" - Resubmitting triggered work");

		k_poll_signal_reset(&triggered_tests[0].signal);
		zassert_true(k_work_poll_submit(&triggered_tests[0].work,
						&triggered_tests[0].event,
						1, K_FOREVER) == 0, NULL);
	}
}

/**
 * @brief Verify a triggered (poll) work item can resubmit itself from its
 * handler.
 *
 * @details
 * A triggered work item handler resets its signal and resubmits the same item
 * each time it runs, until a target count is reached. This confirms a triggered
 * item may be resubmitted from within its own handler and re-runs each time its
 * event is signalled.
 *
 * Test steps:
 * - Initialize a triggered work item whose handler resubmits itself.
 * - Submit it and repeatedly raise its signal up to the target count.
 *
 * Expected result:
 * - The handler runs once per iteration and all results are recorded in order.
 *
 * @see k_work_poll_init(), k_work_poll_submit()
 */
ZTEST(workqueue_triggered, test_triggered_resubmit)
{
	int i;

	LOG_DBG("Starting triggered resubmit test");

	/* As work items are triggered, they should indicate an event. */
	expected_poll_result = 0;

	triggered_tests[0].key = 1;
	k_work_poll_init(&triggered_tests[0].work,
			 triggered_resubmit_work_handler);

	k_poll_signal_init(&triggered_tests[0].signal);
	k_poll_event_init(&triggered_tests[0].event,
			  K_POLL_TYPE_SIGNAL,
			  K_POLL_MODE_NOTIFY_ONLY,
			  &triggered_tests[0].signal);

	LOG_DBG(" - Submitting triggered work");
	zassert_true(k_work_poll_submit(&triggered_tests[0].work,
					&triggered_tests[0].event,
					1, K_FOREVER) == 0, NULL);

	for (i = 0; i < NUM_TEST_ITEMS; i++) {
		LOG_DBG(" - Triggering test item execution (iteration: %d)", i + 1);
		zassert_true(k_poll_signal_raise(&triggered_tests[0].signal,
						 1) == 0, NULL);
		k_msleep(WORK_ITEM_WAIT);
	}

	LOG_DBG(" - Checking results");
	check_results(NUM_TEST_ITEMS);
	reset_results();
}

/**
 * @brief Verify triggered (poll) work items submitted with K_NO_WAIT run when
 * already signalled.
 *
 * @details
 * When a triggered work item is submitted with a K_NO_WAIT timeout and its poll
 * event is already signalled, the item must run rather than expire. This test
 * raises the signals first, then submits with K_NO_WAIT.
 *
 * Test steps:
 * - Initialize the triggered work items.
 * - Raise each signal, then submit the items with a K_NO_WAIT timeout.
 *
 * Expected result:
 * - Every triggered handler runs with a poll result of 0.
 *
 * @see k_work_poll_init(), k_work_poll_submit()
 */
ZTEST(workqueue_triggered, test_triggered_no_wait)
{
	LOG_DBG("Starting triggered test");

	/* As work items are triggered, they should indicate an event. */
	expected_poll_result = 0;

	LOG_DBG(" - Initializing triggered test items");
	test_triggered_init();

	LOG_DBG(" - Triggering test items execution");
	test_triggered_trigger();

	LOG_DBG(" - Submitting triggered test items");
	test_triggered_submit(K_NO_WAIT);

	/* Items should be executed when we will be sleeping. */
	k_msleep(WORK_ITEM_WAIT);

	LOG_DBG(" - Checking results");
	check_results(NUM_TEST_ITEMS);
	reset_results();
}

/**
 * @brief Verify triggered (poll) work items submitted with K_NO_WAIT expire
 * when not signalled.
 *
 * @details
 * When a triggered work item is submitted with a K_NO_WAIT timeout and its poll
 * event is not signalled, the item must run immediately reporting an expired
 * poll result. This test submits with K_NO_WAIT without raising any signal.
 *
 * Test steps:
 * - Initialize the triggered work items.
 * - Submit the items with a K_NO_WAIT timeout and do not raise their signals.
 *
 * Expected result:
 * - Every triggered handler runs with a poll result of -EAGAIN.
 *
 * @see k_work_poll_init(), k_work_poll_submit()
 */
ZTEST(workqueue_triggered, test_triggered_no_wait_expired)
{
	LOG_DBG("Starting triggered test");

	/* As work items are not triggered, they should be marked as expired. */
	expected_poll_result = -EAGAIN;

	LOG_DBG(" - Initializing triggered test items");
	test_triggered_init();

	LOG_DBG(" - Submitting triggered test items");
	test_triggered_submit(K_NO_WAIT);

	/* Items should be executed when we will be sleeping. */
	k_msleep(WORK_ITEM_WAIT);

	LOG_DBG(" - Checking results");
	check_results(NUM_TEST_ITEMS);
	reset_results();
}

/**
 * @brief Verify triggered (poll) work items run when signalled before a finite
 * timeout elapses.
 *
 * @details
 * When a triggered work item is submitted with a finite timeout and its poll
 * event is signalled before the timeout elapses, the item must run reporting a
 * successful poll result. This test raises the signals, then submits with a
 * finite timeout.
 *
 * Test steps:
 * - Initialize the triggered work items.
 * - Raise each signal, then submit the items with a finite timeout.
 *
 * Expected result:
 * - Every triggered handler runs with a poll result of 0.
 *
 * @see k_work_poll_init(), k_work_poll_submit()
 */
ZTEST(workqueue_triggered, test_triggered_wait)
{
	LOG_DBG("Starting triggered test");

	/* As work items are triggered, they should indicate an event. */
	expected_poll_result = 0;

	LOG_DBG(" - Initializing triggered test items");
	test_triggered_init();

	LOG_DBG(" - Triggering test items execution");
	test_triggered_trigger();

	LOG_DBG(" - Submitting triggered test items");
	test_triggered_submit(K_MSEC(2 * SUBMIT_WAIT));

	/* Items should be executed when we will be sleeping. */
	k_msleep(SUBMIT_WAIT);

	LOG_DBG(" - Checking results");
	check_results(NUM_TEST_ITEMS);
	reset_results();
}

/**
 * @brief Verify triggered (poll) work items expire after a finite timeout when
 * never signalled.
 *
 * @details
 * When a triggered work item is submitted with a finite timeout and its poll
 * event is never signalled, the item must not run until the timeout elapses, at
 * which point it runs reporting an expired poll result. This test checks the
 * items have not run before the timeout and have run (as expired) afterwards.
 *
 * Test steps:
 * - Initialize and submit the triggered work items with a finite timeout.
 * - Confirm none have run before the timeout elapses.
 * - Wait past the timeout and confirm all have run.
 *
 * Expected result:
 * - No handler runs before the timeout; afterwards every handler runs with a
 *   poll result of -EAGAIN.
 *
 * @see k_work_poll_init(), k_work_poll_submit()
 */
ZTEST(workqueue_triggered, test_triggered_wait_expired)
{
	LOG_DBG("Starting triggered test");

	/* As work items are not triggered, they should time out. */
	expected_poll_result = -EAGAIN;

	LOG_DBG(" - Initializing triggered test items");
	test_triggered_init();

	LOG_DBG(" - Submitting triggered test items");
	test_triggered_submit(K_MSEC(2 * SUBMIT_WAIT));

	/* Items should not be executed when we will be sleeping here. */
	k_msleep(SUBMIT_WAIT);
	LOG_DBG(" - Checking results (before timeout)");
	check_results(0);

	/* Items should be executed when we will be sleeping here. */
	k_msleep(SUBMIT_WAIT * 2);
	LOG_DBG(" - Checking results (after timeout)");
	check_results(NUM_TEST_ITEMS);

	reset_results();
}


static void msg_provider_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	char msg[MSG_SIZE] = { 0 };

	k_msgq_put(&triggered_from_msgq_test.msgq, &msg, K_NO_WAIT);
}

static void triggered_from_msgq_work_handler(struct k_work *work)
{
	char msg[MSG_SIZE];

	k_msgq_get(&triggered_from_msgq_test.msgq, &msg, K_NO_WAIT);
}

static void test_triggered_from_msgq_init(void)
{
	struct triggered_from_msgq_test_item *const ctx = &triggered_from_msgq_test;

	ctx->tid = k_thread_create(&ctx->msg_provider_thread,
				   provider_thread_stack,
				   MSG_PROVIDER_THREAD_STACK_SIZE,
				   msg_provider_thread,
				   NULL, NULL, NULL,
				   MSG_PROVIDER_THREAD_PRIO, 0, K_FOREVER);
	k_work_queue_init(&ctx->msg_consumer_workq);
	k_msgq_init(&ctx->msgq,
		    (char *)ctx->msgq_buf,
		    MSG_SIZE, 1U);
	k_work_poll_init(&ctx->work, triggered_from_msgq_work_handler);
	k_poll_event_init(&ctx->event, K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
			  K_POLL_MODE_NOTIFY_ONLY, &ctx->msgq);

	k_work_queue_start(&ctx->msg_consumer_workq, consumer_workq_stack,
			   MSG_CONSUMER_WORKQ_STACK_SIZE, MSG_CONSUMER_WORKQ_PRIO,
			   NULL);
	k_work_poll_submit_to_queue(&ctx->msg_consumer_workq, &ctx->work,
				    &ctx->event, 1U, K_FOREVER);
}

static void test_triggered_from_msgq_start(void)
{
	k_thread_start(triggered_from_msgq_test.tid);
}
/**
 * @brief Verify a triggered (poll) work item can be triggered by a message
 * queue becoming readable.
 *
 * @details
 * Regression test for issue #45267: when an object-availability event triggers
 * a k_work_poll item, the triggering object's lock must no longer be held while
 * the work callback runs, so the callback can safely access that object. The
 * test arms a triggered work item on a K_POLL_TYPE_MSGQ_DATA_AVAILABLE event and
 * has a provider thread put a message, whose handler then drains the queue.
 *
 * Test steps:
 * - Initialize a message queue, a consumer work queue and a triggered work item
 *   armed on the message queue's data-available event.
 * - Start a provider thread that puts a message into the queue.
 *
 * Expected result:
 * - The triggered handler runs and consumes the message without deadlocking on
 *   the message queue lock.
 *
 * @see k_work_poll_init(), k_work_poll_submit_to_queue()
 */
ZTEST(workqueue_triggered, test_triggered_from_msgq)
{
	LOG_DBG("Starting triggered from msgq test");

	LOG_DBG(" - Initializing kernel objects");
	test_triggered_from_msgq_init();

	LOG_DBG(" - Starting the thread");
	test_triggered_from_msgq_start();

	reset_results();
}

/**
 * @brief Verify K_WORK_DELAYABLE_DEFINE() initializes a delayable work item
 * identically to k_work_init_delayable().
 *
 * @details
 * A delayable work item can be defined statically with K_WORK_DELAYABLE_DEFINE()
 * or initialized at run time with k_work_init_delayable(). This test confirms
 * both produce a byte-for-byte identical delayable work item for the same
 * handler.
 *
 * Test steps:
 * - Define a delayable work item with K_WORK_DELAYABLE_DEFINE().
 * - Initialize another with k_work_init_delayable() using the same handler.
 * - Compare the two structures.
 *
 * Expected result:
 * - The statically defined and run-time initialized items are identical.
 *
 * @see K_WORK_DELAYABLE_DEFINE(), k_work_init_delayable()
 * @verifies ZEP-SRS-26-20
 */
ZTEST(workqueue_triggered, test_delayed_work_define)
{
	struct k_work_delayable initialized_by_function = { 0 };

	K_WORK_DELAYABLE_DEFINE(initialized_by_macro, delayed_work_handler);

	k_work_init_delayable(&initialized_by_function, delayed_work_handler);

	zassert_mem_equal(&initialized_by_function, &initialized_by_macro,
			  sizeof(struct k_work_delayable), NULL);
}

/**
 * @brief Verify k_work_poll_cancel() return values for triggered work items.
 *
 * @details
 * Cancelling a submitted triggered work item succeeds, while cancelling an item
 * that is no longer cancellable, or a NULL item, must report an error. This
 * test exercises each case in turn.
 *
 * Test steps:
 * - Submit a triggered work item and cancel it with k_work_poll_cancel().
 * - Cancel the same item again.
 * - Cancel a NULL work item.
 *
 * Expected result:
 * - The first cancel returns 0; the repeated cancel and the NULL cancel both
 *   return -EINVAL.
 *
 * @see k_work_poll_cancel()
 */
ZTEST(workqueue_triggered, test_triggered_cancel)
{
	int ret;

	LOG_DBG("Starting triggered test");

	/* As work items are triggered, they should indicate an event. */
	expected_poll_result = 0;

	LOG_DBG(" - Initializing triggered test items");
	test_triggered_init();

	test_triggered_submit(K_FOREVER);

	ret = k_work_poll_cancel(&triggered_tests[0].work);
	zassert_true(ret == 0, "triggered cancel failed");

	ret = k_work_poll_cancel(&triggered_tests[0].work);
	zassert_true(ret == -EINVAL, "triggered cancel failed");

	ret = k_work_poll_cancel(NULL);
	zassert_true(ret == -EINVAL, "triggered cancel failed");
}

/*test case main entry*/
static void *workq_setup(void)
{
	k_thread_priority_set(k_current_get(), 0);
	test_sequence();

	return NULL;
}


ZTEST_SUITE(workqueue_delayed, NULL, workq_setup, ztest_simple_1cpu_before,
		 ztest_simple_1cpu_after, NULL);
ZTEST_SUITE(workqueue_triggered, NULL, workq_setup, ztest_simple_1cpu_before,
		 ztest_simple_1cpu_after, NULL);
