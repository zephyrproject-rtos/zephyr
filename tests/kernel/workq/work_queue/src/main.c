/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

#include <zephyr.h>
#include <ztest.h>
#include <tc_util.h>
#include <misc/util.h>

#define NUM_TEST_ITEMS          6
/* Each work item takes 100ms */
#define WORK_ITEM_WAIT          100

/*
 * Wait 50ms between work submissions, to ensure co-op and prempt
 * preempt thread submit alternatively.
 */
#define SUBMIT_WAIT             50

#define STACK_SIZE      1024

struct test_item {
	int key;
	struct k_delayed_work work;
};

static K_THREAD_STACK_DEFINE(co_op_stack, STACK_SIZE);
static struct k_thread co_op_data;

static struct test_item tests[NUM_TEST_ITEMS];

static int results[NUM_TEST_ITEMS];
static int num_results;

static void work_handler(struct k_work *work)
{
	struct test_item *ti = CONTAINER_OF(work, struct test_item, work);

	TC_PRINT(" - Running test item %d\n", ti->key);
	k_sleep(WORK_ITEM_WAIT);

	results[num_results++] = ti->key;
}

static void test_items_init(void)
{
	int i;

	for (i = 0; i < NUM_TEST_ITEMS; i++) {
		tests[i].key = i + 1;
		k_work_init(&tests[i].work.work, work_handler);
	}
}

static void coop_work_main(int arg1, int arg2)
{
	int i;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	/* Let the preempt thread submit the first work item. */
	k_sleep(SUBMIT_WAIT / 2);

	for (i = 1; i < NUM_TEST_ITEMS; i += 2) {
		TC_PRINT(" - Submitting work %d from coop thread\n", i + 1);
		k_work_submit(&tests[i].work.work);
		k_sleep(SUBMIT_WAIT);
	}
}

static void test_items_submit(void)
{
	int i;

	k_thread_create(&co_op_data, co_op_stack, STACK_SIZE,
			(k_thread_entry_t)coop_work_main,
			NULL, NULL, NULL, K_PRIO_COOP(10), 0, 0);

	for (i = 0; i < NUM_TEST_ITEMS; i += 2) {
		TC_PRINT(" - Submitting work %d from preempt thread\n", i + 1);
		k_work_submit(&tests[i].work.work);
		k_sleep(SUBMIT_WAIT);
	}
}

static int check_results(int num_tests)
{
	int i;

	if (num_results != num_tests) {
		TC_ERROR("*** work items finished: %d (expected: %d)\n",
			 num_results, num_tests);
		return TC_FAIL;
	}

	for (i = 0; i < num_tests; i++) {
		if (results[i] != i + 1) {
			TC_ERROR("*** got result %d in position %d"
				 " (expected %d)\n",
				 results[i], i, i + 1);
			return TC_FAIL;
		}
	}

	return TC_PASS;
}

static int test_sequence(void)
{
	TC_PRINT("Starting sequence test\n");

	TC_PRINT(" - Initializing test items\n");
	test_items_init();

	TC_PRINT(" - Submitting test items\n");
	test_items_submit();

	TC_PRINT(" - Waiting for work to finish\n");
	k_sleep((NUM_TEST_ITEMS + 1) * WORK_ITEM_WAIT);

	TC_PRINT(" - Checking results\n");
	return check_results(NUM_TEST_ITEMS);
}


static void reset_results(void)
{
	int i;

	for (i = 0; i < NUM_TEST_ITEMS; i++)
		results[i] = 0;

	num_results = 0;
}

static void resubmit_work_handler(struct k_work *work)
{
	struct test_item *ti = CONTAINER_OF(work, struct test_item, work);

	k_sleep(WORK_ITEM_WAIT);

	results[num_results++] = ti->key;

	if (ti->key < NUM_TEST_ITEMS) {
		ti->key++;
		TC_PRINT(" - Resubmitting work\n");
		k_work_submit(work);
	}
}

static int test_resubmit(void)
{
	TC_PRINT("Starting resubmit test\n");

	tests[0].key = 1;
	tests[0].work.work.handler = resubmit_work_handler;

	TC_PRINT(" - Submitting work\n");
	k_work_submit(&tests[0].work.work);

	TC_PRINT(" - Waiting for work to finish\n");
	k_sleep((NUM_TEST_ITEMS + 1) * WORK_ITEM_WAIT);

	TC_PRINT(" - Checking results\n");
	return check_results(NUM_TEST_ITEMS);
}

static void delayed_work_handler(struct k_work *work)
{
	struct test_item *ti = CONTAINER_OF(work, struct test_item, work);

	TC_PRINT(" - Running delayed test item %d\n", ti->key);

	results[num_results++] = ti->key;
}

static void test_delayed_init(void)
{
	int i;

	for (i = 0; i < NUM_TEST_ITEMS; i++) {
		tests[i].key = i + 1;
		k_delayed_work_init(&tests[i].work, delayed_work_handler);
	}
}

static void coop_delayed_work_main(int arg1, int arg2)
{
	int i;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	/* Let the preempt thread submit the first work item. */
	k_sleep(SUBMIT_WAIT / 2);

	for (i = 1; i < NUM_TEST_ITEMS; i += 2) {
		TC_PRINT(" - Submitting delayed work %d from"
			 " coop thread\n", i + 1);
		k_delayed_work_submit(&tests[i].work,
				      (i + 1) * WORK_ITEM_WAIT);
	}
}

static int test_delayed_submit(void)
{
	int i;

	k_thread_create(&co_op_data, co_op_stack, STACK_SIZE,
			(k_thread_entry_t)coop_delayed_work_main,
			NULL, NULL, NULL, K_PRIO_COOP(10), 0, 0);

	for (i = 0; i < NUM_TEST_ITEMS; i += 2) {
		TC_PRINT(" - Submitting delayed work %d from"
			 " preempt thread\n", i + 1);
		if (k_delayed_work_submit(&tests[i].work,
					  (i + 1) * WORK_ITEM_WAIT)) {
			return TC_FAIL;
		}
	}

	return TC_PASS;
}

static void coop_delayed_work_cancel_main(int arg1, int arg2)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	k_delayed_work_submit(&tests[1].work, WORK_ITEM_WAIT);

	TC_PRINT(" - Cancel delayed work from coop thread\n");
	k_delayed_work_cancel(&tests[1].work);

#if defined(CONFIG_POLL)
	k_delayed_work_submit(&tests[2].work, 0 /* Submit immeditelly */);

	TC_PRINT(" - Cancel pending delayed work from coop thread\n");
	k_delayed_work_cancel(&tests[2].work);
#endif
}

static int test_delayed_cancel(void)
{
	TC_PRINT("Starting delayed cancel test\n");

	k_delayed_work_submit(&tests[0].work, WORK_ITEM_WAIT);

	TC_PRINT(" - Cancel delayed work from preempt thread\n");
	k_delayed_work_cancel(&tests[0].work);

	k_thread_create(&co_op_data, co_op_stack, STACK_SIZE,
			(k_thread_entry_t)coop_delayed_work_cancel_main,
			NULL, NULL, NULL, K_HIGHEST_THREAD_PRIO, 0, 0);

	TC_PRINT(" - Waiting for work to finish\n");
	k_sleep(2 * WORK_ITEM_WAIT);

	TC_PRINT(" - Checking results\n");
	return check_results(0);
}

static void delayed_resubmit_work_handler(struct k_work *work)
{
	struct test_item *ti = CONTAINER_OF(work, struct test_item, work);

	results[num_results++] = ti->key;

	if (ti->key < NUM_TEST_ITEMS) {
		ti->key++;
		TC_PRINT(" - Resubmitting delayed work\n");
		k_delayed_work_submit(&ti->work, WORK_ITEM_WAIT);
	}
}

static int test_delayed_resubmit(void)
{
	TC_PRINT("Starting delayed resubmit test\n");

	tests[0].key = 1;
	k_delayed_work_init(&tests[0].work, delayed_resubmit_work_handler);

	TC_PRINT(" - Submitting delayed work\n");
	k_delayed_work_submit(&tests[0].work, WORK_ITEM_WAIT);

	TC_PRINT(" - Waiting for work to finish\n");
	k_sleep((NUM_TEST_ITEMS + 1) * WORK_ITEM_WAIT);

	TC_PRINT(" - Checking results\n");
	return check_results(NUM_TEST_ITEMS);
}

static void coop_delayed_work_resubmit(int arg1, int arg2)
{
	int i;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	for (i = 0; i < NUM_TEST_ITEMS; i++) {
		volatile u32_t uptime;

		TC_PRINT(" - Resubmitting delayed work with 1 ms\n");
		k_delayed_work_submit(&tests[0].work, 1);

		/* Busy wait 1 ms to force a clash with workqueue */
		uptime = k_uptime_get_32();
		while (k_uptime_get_32() == uptime) {
		}
	}
}

static int test_delayed_resubmit_thread(void)
{
	TC_PRINT("Starting delayed resubmit from coop thread test\n");

	tests[0].key = 1;
	k_delayed_work_init(&tests[0].work, delayed_work_handler);

	k_thread_create(&co_op_data, co_op_stack, STACK_SIZE,
			(k_thread_entry_t)coop_delayed_work_resubmit,
			NULL, NULL, NULL, K_PRIO_COOP(10), 0, 0);

	TC_PRINT(" - Waiting for work to finish\n");
	k_sleep(NUM_TEST_ITEMS + 1);

	TC_PRINT(" - Checking results\n");
	return check_results(1);
}

static int test_delayed(void)
{
	TC_PRINT("Starting delayed test\n");

	TC_PRINT(" - Initializing delayed test items\n");
	test_delayed_init();

	TC_PRINT(" - Submitting delayed test items\n");
	if (test_delayed_submit() != TC_PASS) {
		return TC_FAIL;
	}

	TC_PRINT(" - Waiting for delayed work to finish\n");
	k_sleep((NUM_TEST_ITEMS + 2) * WORK_ITEM_WAIT);

	TC_PRINT(" - Checking results\n");
	return check_results(NUM_TEST_ITEMS);
}

void testing_workq(void)
{
	/*
	 * Main thread(test_main) priority is 0 but ztest thread runs at
	 * priority -1. To run the test smoothly make both main and ztest
	 * threads run at same priority level.
	 */
	k_thread_priority_set(k_current_get(), 0);

	zassert_equal(test_sequence(), TC_PASS, "test sequence failed");

	reset_results();

	zassert_equal(test_resubmit(), TC_PASS, "test resubmit failed");

	reset_results();

	zassert_equal(test_delayed(), TC_PASS, "test delayed failed");

	reset_results();

	zassert_equal(test_delayed_resubmit(), TC_PASS, "delayed resubmit"
		      " failed");

	reset_results();

	zassert_equal(test_delayed_resubmit_thread(), TC_PASS,
		      "delayed resubmit thread failed");
	reset_results();

	zassert_equal(test_delayed_cancel(), TC_PASS,
		      "delayed cancel failed");
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_workq, ztest_unit_test(testing_workq));
	ztest_run_test_suite(test_workq);
}
