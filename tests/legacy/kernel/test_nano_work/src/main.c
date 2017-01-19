/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

#include <zephyr.h>
#include <nanokernel.h>
#include <tc_util.h>
#include <misc/util.h>
#include <misc/nano_work.h>

#define NUM_TEST_ITEMS		6
/* Each work item takes 100ms */
#define WORK_ITEM_WAIT		(sys_clock_ticks_per_sec / 10)

/*
 * Wait 50ms between work submissions, to esure fiber and task submit
 * alternatively.
 */
#define SUBMIT_WAIT		(sys_clock_ticks_per_sec / 20)

#define FIBER_STACK_SIZE	1024

struct test_item {
	int key;
	struct nano_delayed_work work;
};

static char __stack fiber_stack[FIBER_STACK_SIZE];

static struct test_item tests[NUM_TEST_ITEMS];

static int results[NUM_TEST_ITEMS];
static int num_results;

static void work_handler(struct nano_work *work)
{
	struct test_item *ti = CONTAINER_OF(work, struct test_item, work);

	TC_PRINT(" - Running test item %d\n", ti->key);
	fiber_sleep(WORK_ITEM_WAIT);

	results[num_results++] = ti->key;
}

static void test_items_init(void)
{
	int i;

	for (i = 0; i < NUM_TEST_ITEMS; i++) {
		tests[i].key = i + 1;
		nano_work_init(&tests[i].work.work, work_handler);
	}
}

static void fiber_work_main(int arg1, int arg2)
{
	int i;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	/* Let the task submit the first work item. */
	fiber_sleep(SUBMIT_WAIT / 2);

	for (i = 1; i < NUM_TEST_ITEMS; i += 2) {
		TC_PRINT(" - Submitting work %d from fiber\n", i + 1);
		nano_work_submit(&tests[i].work.work);
		fiber_sleep(SUBMIT_WAIT);
	}
}

static void test_items_submit(void)
{
	int i;

	task_fiber_start(fiber_stack, FIBER_STACK_SIZE,
			 fiber_work_main, 0, 0, 10, 0);

	for (i = 0; i < NUM_TEST_ITEMS; i += 2) {
		TC_PRINT(" - Submitting work %d from task\n", i + 1);
		nano_work_submit(&tests[i].work.work);
		task_sleep(SUBMIT_WAIT);
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
			TC_ERROR("*** got result %d in position %d (expected %d)\n",
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
	task_sleep((NUM_TEST_ITEMS + 1) * WORK_ITEM_WAIT);

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

static void resubmit_work_handler(struct nano_work *work)
{
	struct test_item *ti = CONTAINER_OF(work, struct test_item, work);

	fiber_sleep(WORK_ITEM_WAIT);

	results[num_results++] = ti->key;

	if (ti->key < NUM_TEST_ITEMS) {
		ti->key++;
		TC_PRINT(" - Resubmitting work\n");
		nano_work_submit(work);
	}
}

static int test_resubmit(void)
{
	TC_PRINT("Starting resubmit test\n");

	tests[0].key = 1;
	tests[0].work.work.handler = resubmit_work_handler;

	TC_PRINT(" - Submitting work\n");
	nano_work_submit(&tests[0].work.work);

	TC_PRINT(" - Waiting for work to finish\n");
	task_sleep((NUM_TEST_ITEMS + 1) * WORK_ITEM_WAIT);

	TC_PRINT(" - Checking results\n");
	return check_results(NUM_TEST_ITEMS);
}

static void delayed_work_handler(struct nano_work *work)
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
		nano_delayed_work_init(&tests[i].work, delayed_work_handler);
	}
}

static void fiber_delayed_work_main(int arg1, int arg2)
{
	int i;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	/* Let the task submit the first work item. */
	fiber_sleep(SUBMIT_WAIT / 2);

	for (i = 1; i < NUM_TEST_ITEMS; i += 2) {
		TC_PRINT(" - Submitting delayed work %d from fiber\n", i + 1);
		nano_delayed_work_submit(&tests[i].work,
					 (i + 1) * WORK_ITEM_WAIT);
	}
}

static int test_delayed_submit(void)
{
	int i;

	task_fiber_start(fiber_stack, FIBER_STACK_SIZE,
			 fiber_delayed_work_main, 0, 0, 10, 0);

	for (i = 0; i < NUM_TEST_ITEMS; i += 2) {
		TC_PRINT(" - Submitting delayed work %d from task\n", i + 1);
		if (nano_delayed_work_submit(&tests[i].work,
					     (i + 1) * WORK_ITEM_WAIT)) {
			return TC_FAIL;
		}
	}

	return TC_PASS;
}

static void fiber_delayed_work_cancel_main(int arg1, int arg2)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	nano_delayed_work_submit(&tests[1].work, WORK_ITEM_WAIT);

	TC_PRINT(" - Cancel delayed work from fiber\n");
	nano_delayed_work_cancel(&tests[1].work);
}

static int test_delayed_cancel(void)
{
	TC_PRINT("Starting delayed cancel test\n");

	nano_delayed_work_submit(&tests[0].work, WORK_ITEM_WAIT);

	TC_PRINT(" - Cancel delayed work from task\n");
	nano_delayed_work_cancel(&tests[0].work);

	task_fiber_start(fiber_stack, FIBER_STACK_SIZE,
			 fiber_delayed_work_cancel_main, 0, 0, 10, 0);

	TC_PRINT(" - Waiting for work to finish\n");
	task_sleep(2 * WORK_ITEM_WAIT);

	TC_PRINT(" - Checking results\n");
	return check_results(0);
}

static void delayed_resubmit_work_handler(struct nano_work *work)
{
	struct test_item *ti = CONTAINER_OF(work, struct test_item, work);

	results[num_results++] = ti->key;

	if (ti->key < NUM_TEST_ITEMS) {
		ti->key++;
		TC_PRINT(" - Resubmitting delayed work\n");
		nano_delayed_work_submit(&ti->work, WORK_ITEM_WAIT);
	}
}

static int test_delayed_resubmit(void)
{
	TC_PRINT("Starting delayed resubmit test\n");

	tests[0].key = 1;
	nano_delayed_work_init(&tests[0].work, delayed_resubmit_work_handler);

	TC_PRINT(" - Submitting delayed work\n");
	nano_delayed_work_submit(&tests[0].work, WORK_ITEM_WAIT);

	TC_PRINT(" - Waiting for work to finish\n");
	task_sleep((NUM_TEST_ITEMS + 1) * WORK_ITEM_WAIT);

	TC_PRINT(" - Checking results\n");
	return check_results(NUM_TEST_ITEMS);
}

static void fiber_delayed_work_resubmit(int arg1, int arg2)
{
	int i;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	for (i = 0; i < NUM_TEST_ITEMS; i++) {
		int ticks;

		TC_PRINT(" - Resubmitting delayed work with 1 tick\n");
		nano_delayed_work_submit(&tests[0].work, 1);

		/* Busy wait 1 tick to force a clash with workqueue */
		ticks = sys_tick_get_32();
		while (sys_tick_get_32() == ticks) {
		}
	}
}

static int test_delayed_resubmit_fiber(void)
{
	TC_PRINT("Starting delayed resubmit from fiber test\n");

	tests[0].key = 1;
	nano_delayed_work_init(&tests[0].work, delayed_work_handler);

	task_fiber_start(fiber_stack, FIBER_STACK_SIZE,
			 fiber_delayed_work_resubmit, 0, 0, 10, 0);

	TC_PRINT(" - Waiting for work to finish\n");
	task_sleep(NUM_TEST_ITEMS + 1);

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
	task_sleep((NUM_TEST_ITEMS + 2) * WORK_ITEM_WAIT);

	TC_PRINT(" - Checking results\n");
	return check_results(NUM_TEST_ITEMS);
}

void main(void)
{
	int status = TC_FAIL;

	if (test_sequence() != TC_PASS) {
		goto end;
	}

	reset_results();

	if (test_resubmit() != TC_PASS) {
		goto end;
	}

	reset_results();

	if (test_delayed() != TC_PASS) {
		goto end;
	}

	reset_results();

	if (test_delayed_resubmit() != TC_PASS) {
		goto end;
	}

	reset_results();

	if (test_delayed_resubmit_fiber() != TC_PASS) {
		goto end;
	}

	reset_results();

	if (test_delayed_cancel() != TC_PASS) {
		goto end;
	}

	status = TC_PASS;

end:
	TC_END_RESULT(status);
	TC_END_REPORT(status);
}
