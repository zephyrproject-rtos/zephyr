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

/* In fact, each work item could take up to this value */
#define WORK_ITEM_WAIT_ALIGNED	\
	__ticks_to_ms(_ms_to_ticks(WORK_ITEM_WAIT) + _TICK_ALIGN)

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

/**
 * @ingroup kernel_workqueue_tests
 * @see k_work_init()
 */
static void test_items_init(void)
{
	int i;

	for (i = 0; i < NUM_TEST_ITEMS; i++) {
		tests[i].key = i + 1;
		k_work_init(&tests[i].work.work, work_handler);
	}
}

static void reset_results(void)
{
	int i;

	for (i = 0; i < NUM_TEST_ITEMS; i++)
		results[i] = 0;

	num_results = 0;
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

/**
 * @ingroup kernel_workqueue_tests
 * @see k_work_submit()
 */
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
	test_items_init();

	TC_PRINT(" - Submitting test items\n");
	test_items_submit();

	TC_PRINT(" - Waiting for work to finish\n");
	k_sleep(NUM_TEST_ITEMS * WORK_ITEM_WAIT_ALIGNED);

	check_results(NUM_TEST_ITEMS);
	reset_results();
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

	tests[0].key = 1;
	tests[0].work.work.handler = resubmit_work_handler;

	TC_PRINT(" - Submitting work\n");
	k_work_submit(&tests[0].work.work);

	TC_PRINT(" - Waiting for work to finish\n");
	k_sleep(NUM_TEST_ITEMS * WORK_ITEM_WAIT_ALIGNED);

	TC_PRINT(" - Checking results\n");
	check_results(NUM_TEST_ITEMS);
	reset_results();
}

static void delayed_work_handler(struct k_work *work)
{
	struct test_item *ti = CONTAINER_OF(work, struct test_item, work);

	TC_PRINT(" - Running delayed test item %d\n", ti->key);

	results[num_results++] = ti->key;
}

/**
 * @brief Test delayed work queue init
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_delayed_work_init()
 */
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

/**
 * @brief Test delayed workqueue submit
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_delayed_work_init(), k_delayed_work_submit()
 */
static void test_delayed_submit(void)
{
	int i;

	k_thread_create(&co_op_data, co_op_stack, STACK_SIZE,
			(k_thread_entry_t)coop_delayed_work_main,
			NULL, NULL, NULL, K_PRIO_COOP(10), 0, 0);

	for (i = 0; i < NUM_TEST_ITEMS; i += 2) {
		TC_PRINT(" - Submitting delayed work %d from"
			 " preempt thread\n", i + 1);
		zassert_true(k_delayed_work_submit(&tests[i].work,
						   (i + 1) * WORK_ITEM_WAIT) == 0, NULL);
	}

}

static void coop_delayed_work_cancel_main(int arg1, int arg2)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	k_delayed_work_submit(&tests[1].work, WORK_ITEM_WAIT);

	TC_PRINT(" - Cancel delayed work from coop thread\n");
	k_delayed_work_cancel(&tests[1].work);

#if defined(CONFIG_POLL)
	k_delayed_work_submit(&tests[2].work, 0 /* Submit immediately */);

	TC_PRINT(" - Cancel pending delayed work from coop thread\n");
	k_delayed_work_cancel(&tests[2].work);
#endif
}

/**
 * @brief Test work queue delayed cancel
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_delayed_work_init(), k_delayed_work_submit(),
 * k_delayed_work_cancel()
 */
static void test_delayed_cancel(void)
{
	TC_PRINT("Starting delayed cancel test\n");

	k_delayed_work_submit(&tests[0].work, WORK_ITEM_WAIT);

	TC_PRINT(" - Cancel delayed work from preempt thread\n");
	k_delayed_work_cancel(&tests[0].work);

	k_thread_create(&co_op_data, co_op_stack, STACK_SIZE,
			(k_thread_entry_t)coop_delayed_work_cancel_main,
			NULL, NULL, NULL, K_HIGHEST_THREAD_PRIO, 0, 0);

	TC_PRINT(" - Waiting for work to finish\n");
	k_sleep(WORK_ITEM_WAIT_ALIGNED);

	TC_PRINT(" - Checking results\n");
	check_results(0);
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

/**
 * @brief Test delayed resubmission of work queue item
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_delayed_work_init(), k_delayed_work_submit()
 */
static void test_delayed_resubmit(void)
{
	TC_PRINT("Starting delayed resubmit test\n");

	tests[0].key = 1;
	k_delayed_work_init(&tests[0].work, delayed_resubmit_work_handler);

	TC_PRINT(" - Submitting delayed work\n");
	k_delayed_work_submit(&tests[0].work, WORK_ITEM_WAIT);

	TC_PRINT(" - Waiting for work to finish\n");
	k_sleep(NUM_TEST_ITEMS * WORK_ITEM_WAIT_ALIGNED);

	TC_PRINT(" - Checking results\n");
	check_results(NUM_TEST_ITEMS);
	reset_results();
}

static void coop_delayed_work_resubmit(void)
{
	int i;

	for (i = 0; i < NUM_TEST_ITEMS; i++) {
		TC_PRINT(" - Resubmitting delayed work with 1 ms\n");
		k_delayed_work_submit(&tests[0].work, 1);

		/* Busy wait 1 ms to force a clash with workqueue */
#if defined(CONFIG_ARCH_POSIX)
		k_busy_wait(1000);
#else
		volatile u32_t uptime;
		uptime = k_uptime_get_32();
		while (k_uptime_get_32() == uptime) {
		}
#endif
	}
}


/**
 * @brief Test delayed resubmission of work queue thread
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_delayed_work_init()
 */
static void test_delayed_resubmit_thread(void)
{
	TC_PRINT("Starting delayed resubmit from coop thread test\n");

	tests[0].key = 1;
	k_delayed_work_init(&tests[0].work, delayed_work_handler);

	k_thread_create(&co_op_data, co_op_stack, STACK_SIZE,
			(k_thread_entry_t)coop_delayed_work_resubmit,
			NULL, NULL, NULL, K_PRIO_COOP(10), 0, 0);

	TC_PRINT(" - Waiting for work to finish\n");
	k_sleep(WORK_ITEM_WAIT_ALIGNED);

	TC_PRINT(" - Checking results\n");
	check_results(1);
	reset_results();
}

/**
 * @brief Test delayed work items
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_delayed_work_init(), k_delayed_work_submit()
 */
static void test_delayed(void)
{
	TC_PRINT("Starting delayed test\n");

	TC_PRINT(" - Initializing delayed test items\n");
	test_delayed_init();

	TC_PRINT(" - Submitting delayed test items\n");
	test_delayed_submit();

	TC_PRINT(" - Waiting for delayed work to finish\n");
	k_sleep(NUM_TEST_ITEMS * WORK_ITEM_WAIT_ALIGNED);

	TC_PRINT(" - Checking results\n");
	check_results(NUM_TEST_ITEMS);
	reset_results();
}

/*test case main entry*/
void test_main(void)
{
	k_thread_priority_set(k_current_get(), 0);
	ztest_test_suite(workqueue,
			 ztest_unit_test(test_sequence),
			 ztest_unit_test(test_resubmit),
			 ztest_unit_test(test_delayed),
			 ztest_unit_test(test_delayed_resubmit),
			 ztest_unit_test(test_delayed_resubmit_thread),
			 ztest_unit_test(test_delayed_cancel)
			 );
	ztest_run_test_suite(workqueue);
}
