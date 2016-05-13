/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
	struct nano_work work;
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
		nano_work_init(&tests[i].work, work_handler);
	}
}

static void fiber_main(int arg1, int arg2)
{
	int i;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	/* Let the task submit the first work item. */
	fiber_sleep(SUBMIT_WAIT / 2);

	for (i = 1; i < NUM_TEST_ITEMS; i += 2) {
		TC_PRINT(" - Submitting work %d from fiber\n", i + 1);
		nano_work_submit(&tests[i].work);
		fiber_sleep(SUBMIT_WAIT);
	}
}

static void test_items_submit(void)
{
	int i;

	task_fiber_start(fiber_stack, FIBER_STACK_SIZE,
			 fiber_main, 0, 0, 10, 0);

	for (i = 0; i < NUM_TEST_ITEMS; i += 2) {
		TC_PRINT(" - Submitting work %d from task\n", i + 1);
		nano_work_submit(&tests[i].work);
		task_sleep(SUBMIT_WAIT);
	}
}

static int check_results(void)
{
	int i;

	if (num_results != NUM_TEST_ITEMS) {
		TC_ERROR("*** work items finished: %d (expected: %d)\n",
			 num_results, NUM_TEST_ITEMS);
		return TC_FAIL;
	}

	for (i = 0; i < NUM_TEST_ITEMS; i++) {
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
	return check_results();
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
	tests[0].work.handler = resubmit_work_handler;

	TC_PRINT(" - Submitting work\n");
	nano_work_submit(&tests[0].work);

	TC_PRINT(" - Waiting for work to finish\n");
	task_sleep((NUM_TEST_ITEMS + 1) * WORK_ITEM_WAIT);

	TC_PRINT(" - Checking results\n");
	return check_results();
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

	status = TC_PASS;

end:
	TC_END_RESULT(status);
	TC_END_REPORT(status);
}
