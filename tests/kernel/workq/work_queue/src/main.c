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
#undef __deprecated
#define __deprecated
#undef __DEPRECATED_MACRO
#define __DEPRECATED_MACRO

#include <zephyr/zephyr.h>
#include <ztest.h>
#include <tc_util.h>
#include <zephyr/sys/util.h>

#define NUM_TEST_ITEMS          6
/* Each work item takes 100ms */
#define WORK_ITEM_WAIT          100

/* In fact, each work item could take up to this value */
#define WORK_ITEM_WAIT_ALIGNED	\
	k_ticks_to_ms_floor64(k_ms_to_ticks_ceil32(WORK_ITEM_WAIT) + _TICK_ALIGN)

/*
 * Wait 50ms between work submissions, to ensure co-op and prempt
 * preempt thread submit alternatively.
 */
#define SUBMIT_WAIT	50
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

#define MSG_PROVIDER_THREAD_STACK_SIZE 0x400U
#define MSG_CONSUMER_WORKQ_STACK_SIZE 0x400U

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
	struct delayed_test_item *ti =
			CONTAINER_OF(work, struct delayed_test_item, work);

	TC_PRINT(" - Running test item %d\n", ti->key);
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

static void coop_work_main(int arg1, int arg2)
{
	int i;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	/* Let the preempt thread submit the first work item. */
	k_msleep(SUBMIT_WAIT / 2);

	for (i = 1; i < NUM_TEST_ITEMS; i += 2) {
		TC_PRINT(" - Submitting work %d from coop thread\n", i + 1);
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
			(k_thread_entry_t)coop_work_main,
			NULL, NULL, NULL, K_PRIO_COOP(10), 0, K_NO_WAIT);

	for (i = 0; i < NUM_TEST_ITEMS; i += 2) {
		TC_PRINT(" - Submitting work %d from preempt thread\n", i + 1);
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
	TC_PRINT(" - Initializing test items\n");
	delayed_test_items_init();

	TC_PRINT(" - Submitting test items\n");
	delayed_test_items_submit();

	TC_PRINT(" - Waiting for work to finish\n");
	k_msleep(CHECK_WAIT);

	check_results(NUM_TEST_ITEMS);
	reset_results();
}



static void resubmit_work_handler(struct k_work *work)
{
	struct delayed_test_item *ti =
			CONTAINER_OF(work, struct delayed_test_item, work);

	k_msleep(WORK_ITEM_WAIT);

	results[num_results++] = ti->key;

	if (ti->key < NUM_TEST_ITEMS) {
		ti->key++;
		TC_PRINT(" - Resubmitting work\n");
		k_work_submit(work);
	}
}
/**
 * @brief Test work queue item resubmission
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_submit()
 */
static void test_resubmit(void)
{
	TC_PRINT("Starting resubmit test\n");

	delayed_tests[0].key = 1;
	k_work_init_delayable(&delayed_tests[0].work, resubmit_work_handler);

	TC_PRINT(" - Submitting work\n");
	k_work_schedule(&delayed_tests[0].work, K_NO_WAIT);

	TC_PRINT(" - Waiting for work to finish\n");
	k_msleep(CHECK_WAIT);

	TC_PRINT(" - Checking results\n");
	check_results(NUM_TEST_ITEMS);
	reset_results();
}

static void delayed_work_handler(struct k_work *work)
{
	struct delayed_test_item *ti =
			CONTAINER_OF(work, struct delayed_test_item, work);

	TC_PRINT(" - Running delayed test item %d\n", ti->key);

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

static void coop_delayed_work_main(int arg1, int arg2)
{
	int i;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	/* Let the preempt thread submit the first work item. */
	k_msleep(SUBMIT_WAIT / 2);

	for (i = 1; i < NUM_TEST_ITEMS; i += 2) {
		TC_PRINT(" - Submitting delayed work %d from"
			 " coop thread\n", i + 1);
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
			(k_thread_entry_t)coop_delayed_work_main,
			NULL, NULL, NULL, K_PRIO_COOP(10), 0, K_NO_WAIT);

	for (i = 0; i < NUM_TEST_ITEMS; i += 2) {
		TC_PRINT(" - Submitting delayed work %d from"
			 " preempt thread\n", i + 1);
		zassert_true(k_work_reschedule(&delayed_tests[i].work,
			     K_MSEC((i + 1) * WORK_ITEM_WAIT)) >= 0, NULL);
	}

}

static void coop_delayed_work_cancel_main(int arg1, int arg2)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	k_work_schedule(&delayed_tests[1].work, K_MSEC(WORK_ITEM_WAIT));

	TC_PRINT(" - Cancel delayed work from coop thread\n");
	k_work_cancel_delayable(&delayed_tests[1].work);
}

/**
 * @brief Test work queue delayed cancel
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_delayable_init(), k_work_schedule(),
 * k_work_cancel_delayable()
 */
static void test_delayed_cancel(void)
{
	TC_PRINT("Starting delayed cancel test\n");

	k_work_schedule(&delayed_tests[0].work, K_MSEC(WORK_ITEM_WAIT));

	TC_PRINT(" - Cancel delayed work from preempt thread\n");
	k_work_cancel_delayable(&delayed_tests[0].work);

	k_thread_create(&co_op_data, co_op_stack, STACK_SIZE,
			(k_thread_entry_t)coop_delayed_work_cancel_main,
			NULL, NULL, NULL, K_HIGHEST_THREAD_PRIO, 0, K_NO_WAIT);

	TC_PRINT(" - Waiting for work to finish\n");
	k_msleep(WORK_ITEM_WAIT_ALIGNED);

	TC_PRINT(" - Checking results\n");
	check_results(0);
	reset_results();
}

static void test_delayed_pending(void)
{
	TC_PRINT("Starting delayed pending test\n");

	k_work_init_delayable(&delayed_tests[0].work, delayed_work_handler);

	zassert_false(k_work_delayable_is_pending(&delayed_tests[0].work), NULL);

	TC_PRINT(" - Check pending delayed work when in workqueue\n");
	k_work_schedule(&delayed_tests[0].work, K_NO_WAIT);
	zassert_true(k_work_delayable_is_pending(&delayed_tests[0].work), NULL);

	k_msleep(1);
	zassert_false(k_work_delayable_is_pending(&delayed_tests[0].work), NULL);

	TC_PRINT(" - Checking results\n");
	check_results(1);
	reset_results();

	TC_PRINT(" - Check pending delayed work with timeout\n");
	k_work_schedule(&delayed_tests[0].work, K_MSEC(WORK_ITEM_WAIT));
	zassert_true(k_work_delayable_is_pending(&delayed_tests[0].work), NULL);

	k_msleep(WORK_ITEM_WAIT_ALIGNED);
	zassert_false(k_work_delayable_is_pending(&delayed_tests[0].work), NULL);

	TC_PRINT(" - Checking results\n");
	check_results(1);
	reset_results();
}

/**
 * @brief Test delayed work items
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_init_delayable(), k_work_schedule()
 */
static void test_delayed(void)
{
	TC_PRINT("Starting delayed test\n");

	TC_PRINT(" - Initializing delayed test items\n");
	test_delayed_init();

	TC_PRINT(" - Submitting delayed test items\n");
	test_delayed_submit();

	TC_PRINT(" - Waiting for delayed work to finish\n");
	k_msleep(CHECK_WAIT);

	TC_PRINT(" - Checking results\n");
	check_results(NUM_TEST_ITEMS);
	reset_results();
}

static void triggered_work_handler(struct k_work *work)
{
	struct triggered_test_item *ti =
			CONTAINER_OF(work, struct triggered_test_item, work);

	TC_PRINT(" - Running triggered test item %d\n", ti->key);

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
		TC_PRINT(" - Submitting triggered work %d\n", i + 1);
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
		TC_PRINT(" - Triggering work %d\n", i + 1);
		zassert_true(k_poll_signal_raise(&triggered_tests[i].signal,
						 1) == 0, NULL);
	}
}

/**
 * @brief Test triggered work items
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_poll_init(), k_work_poll_submit()
 */
static void test_triggered(void)
{
	TC_PRINT("Starting triggered test\n");

	/* As work items are triggered, they should indicate an event. */
	expected_poll_result = 0;

	TC_PRINT(" - Initializing triggered test items\n");
	test_triggered_init();

	TC_PRINT(" - Submitting triggered test items\n");
	test_triggered_submit(K_FOREVER);

	TC_PRINT(" - Triggering test items execution\n");
	test_triggered_trigger();

	/* Items should be executed when we will be sleeping. */
	k_msleep(WORK_ITEM_WAIT);

	TC_PRINT(" - Checking results\n");
	check_results(NUM_TEST_ITEMS);
	reset_results();
}

/**
 * @brief Test already triggered work items
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_poll_init(), k_work_poll_submit()
 */
static void test_already_triggered(void)
{
	TC_PRINT("Starting triggered test\n");

	/* As work items are triggered, they should indicate an event. */
	expected_poll_result = 0;

	TC_PRINT(" - Initializing triggered test items\n");
	test_triggered_init();

	TC_PRINT(" - Triggering test items execution\n");
	test_triggered_trigger();

	TC_PRINT(" - Submitting triggered test items\n");
	test_triggered_submit(K_FOREVER);

	/* Items should be executed when we will be sleeping. */
	k_msleep(WORK_ITEM_WAIT);

	TC_PRINT(" - Checking results\n");
	check_results(NUM_TEST_ITEMS);
	reset_results();
}

static void triggered_resubmit_work_handler(struct k_work *work)
{
	struct triggered_test_item *ti =
			CONTAINER_OF(work, struct triggered_test_item, work);

	results[num_results++] = ti->key;

	if (ti->key < NUM_TEST_ITEMS) {
		ti->key++;
		TC_PRINT(" - Resubmitting triggered work\n");

		k_poll_signal_reset(&triggered_tests[0].signal);
		zassert_true(k_work_poll_submit(&triggered_tests[0].work,
						&triggered_tests[0].event,
						1, K_FOREVER) == 0, NULL);
	}
}

/**
 * @brief Test resubmission of triggered work queue item
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_poll_init(), k_work_poll_submit()
 */
static void test_triggered_resubmit(void)
{
	int i;

	TC_PRINT("Starting triggered resubmit test\n");

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

	TC_PRINT(" - Submitting triggered work\n");
	zassert_true(k_work_poll_submit(&triggered_tests[0].work,
					&triggered_tests[0].event,
					1, K_FOREVER) == 0, NULL);

	for (i = 0; i < NUM_TEST_ITEMS; i++) {
		TC_PRINT(" - Triggering test item execution (iteration: %d)\n",
									i + 1);
		zassert_true(k_poll_signal_raise(&triggered_tests[0].signal,
						 1) == 0, NULL);
		k_msleep(WORK_ITEM_WAIT);
	}

	TC_PRINT(" - Checking results\n");
	check_results(NUM_TEST_ITEMS);
	reset_results();
}

/**
 * @brief Test triggered work items with K_NO_WAIT timeout
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_poll_init(), k_work_poll_submit()
 */
static void test_triggered_no_wait(void)
{
	TC_PRINT("Starting triggered test\n");

	/* As work items are triggered, they should indicate an event. */
	expected_poll_result = 0;

	TC_PRINT(" - Initializing triggered test items\n");
	test_triggered_init();

	TC_PRINT(" - Triggering test items execution\n");
	test_triggered_trigger();

	TC_PRINT(" - Submitting triggered test items\n");
	test_triggered_submit(K_NO_WAIT);

	/* Items should be executed when we will be sleeping. */
	k_msleep(WORK_ITEM_WAIT);

	TC_PRINT(" - Checking results\n");
	check_results(NUM_TEST_ITEMS);
	reset_results();
}

/**
 * @brief Test expired triggered work items with K_NO_WAIT timeout
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_poll_init(), k_work_poll_submit()
 */
static void test_triggered_no_wait_expired(void)
{
	TC_PRINT("Starting triggered test\n");

	/* As work items are not triggered, they should be marked as expired. */
	expected_poll_result = -EAGAIN;

	TC_PRINT(" - Initializing triggered test items\n");
	test_triggered_init();

	TC_PRINT(" - Submitting triggered test items\n");
	test_triggered_submit(K_NO_WAIT);

	/* Items should be executed when we will be sleeping. */
	k_msleep(WORK_ITEM_WAIT);

	TC_PRINT(" - Checking results\n");
	check_results(NUM_TEST_ITEMS);
	reset_results();
}

/**
 * @brief Test triggered work items with arbitrary timeout
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_poll_init(), k_work_poll_submit()
 */
static void test_triggered_wait(void)
{
	TC_PRINT("Starting triggered test\n");

	/* As work items are triggered, they should indicate an event. */
	expected_poll_result = 0;

	TC_PRINT(" - Initializing triggered test items\n");
	test_triggered_init();

	TC_PRINT(" - Triggering test items execution\n");
	test_triggered_trigger();

	TC_PRINT(" - Submitting triggered test items\n");
	test_triggered_submit(K_MSEC(2 * SUBMIT_WAIT));

	/* Items should be executed when we will be sleeping. */
	k_msleep(SUBMIT_WAIT);

	TC_PRINT(" - Checking results\n");
	check_results(NUM_TEST_ITEMS);
	reset_results();
}

/**
 * @brief Test expired triggered work items with arbitrary timeout
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_poll_init(), k_work_poll_submit()
 */
static void test_triggered_wait_expired(void)
{
	TC_PRINT("Starting triggered test\n");

	/* As work items are not triggered, they should time out. */
	expected_poll_result = -EAGAIN;

	TC_PRINT(" - Initializing triggered test items\n");
	test_triggered_init();

	TC_PRINT(" - Submitting triggered test items\n");
	test_triggered_submit(K_MSEC(2 * SUBMIT_WAIT));

	/* Items should not be executed when we will be sleeping here. */
	k_msleep(SUBMIT_WAIT);
	TC_PRINT(" - Checking results (before timeout)\n");
	check_results(0);

	/* Items should be executed when we will be sleeping here. */
	k_msleep(SUBMIT_WAIT * 2);
	TC_PRINT(" - Checking results (after timeout)\n");
	check_results(NUM_TEST_ITEMS);

	reset_results();
}


static void msg_provider_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	char msg[MSG_SIZE];

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
 * @brief Test triggered work item, triggered by a msgq message.
 *
 * Regression test for issue #45267:
 *
 * When an object availability event triggers a k_work_poll item,
 * the object lock should not be held anymore during the execution
 * of the work callback.
 *
 * Tested with msgq with K_POLL_TYPE_MSGQ_DATA_AVAILABLE.
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_poll_init(), k_work_poll_submit()
 *
 */
static void test_triggered_from_msgq(void)
{
	TC_PRINT("Starting triggered from msgq test\n");

	TC_PRINT(" - Initializing kernel objects\n");
	test_triggered_from_msgq_init();

	TC_PRINT(" - Starting the thread\n");
	test_triggered_from_msgq_start();

	reset_results();
}

/**
 * @brief Test delayed work queue define macro.
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see K_WORK_DELAYABLE_DEFINE()
 */
void test_delayed_work_define(void)
{
	struct k_work_delayable initialized_by_function = { 0 };

	K_WORK_DELAYABLE_DEFINE(initialized_by_macro, delayed_work_handler);

	k_work_init_delayable(&initialized_by_function, delayed_work_handler);

	zassert_mem_equal(&initialized_by_function, &initialized_by_macro,
			  sizeof(struct k_work_delayable), NULL);
}

/**
 * @brief Verify k_work_poll_cancel()
 *
 * @ingroup kernel_workqueue_tests
 *
 * @details Cancel a triggered work item repeatedly,
 * see if it returns expected value.
 *
 * @see k_work_poll_cancel()
 */
static void test_triggered_cancel(void)
{
	int ret;

	TC_PRINT("Starting triggered test\n");

	/* As work items are triggered, they should indicate an event. */
	expected_poll_result = 0;

	TC_PRINT(" - Initializing triggered test items\n");
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
void test_main(void)
{
	k_thread_priority_set(k_current_get(), 0);
	ztest_test_suite(workqueue,
			 ztest_1cpu_unit_test(test_sequence),
			 ztest_1cpu_unit_test(test_resubmit),
			 ztest_1cpu_unit_test(test_delayed),
			 ztest_1cpu_unit_test(test_delayed_cancel),
			 ztest_1cpu_unit_test(test_delayed_pending),
			 ztest_1cpu_unit_test(test_triggered),
			 ztest_1cpu_unit_test(test_already_triggered),
			 ztest_1cpu_unit_test(test_triggered_resubmit),
			 ztest_1cpu_unit_test(test_triggered_no_wait),
			 ztest_1cpu_unit_test(test_triggered_no_wait_expired),
			 ztest_1cpu_unit_test(test_triggered_wait),
			 ztest_1cpu_unit_test(test_triggered_wait_expired),
			 ztest_1cpu_unit_test(test_triggered_from_msgq),
			 ztest_1cpu_unit_test(test_delayed_work_define),
			 ztest_1cpu_unit_test(test_triggered_cancel)
			 );
	ztest_run_test_suite(workqueue);
}
