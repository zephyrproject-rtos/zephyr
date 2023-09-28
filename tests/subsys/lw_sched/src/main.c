/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/lw_sched/lw_sched.h>

#define INC1  1
#define INC2  2

#define TICKS_PER_INTERVAL  10

struct execute_args {
	uint32_t  data;
};

struct abort_args {
	bool called;
};

/* Define information for lightweight task #1 */

static int task1_handler(void *arg);

static struct execute_args exe_args1;
static struct abort_args   abort_args1;

static struct lw_task_ops  ops1 = {
	.execute = task1_handler,
	.abort   = NULL
};
static struct lw_task_args  args1 = {
	.execute = &exe_args1,
	.abort   = NULL
};

static struct lw_task task1;

/* Define information for lightweight task #2 */

static int task2_handler(void *arg);

static struct execute_args exe_args2;
static struct abort_args   abort_args2;

static struct lw_task_ops  ops2 = {
	.execute = task2_handler,
	.abort   = NULL
};
static struct lw_task_args  args2 = {
	.execute = &exe_args2,
	.abort   = NULL
};

static struct lw_task task2;

K_THREAD_STACK_DEFINE(sched_stack, 1024);

static struct lw_scheduler test_sched;
K_THREAD_STACK_DEFINE(test_sched_stack, 1024);

static void abort_handler(void *arg)
{
	struct abort_args *abort_arg = arg;

	abort_arg->called = true;
}

static int task1_handler(void *arg)
{
	struct execute_args *data = arg;

	data->data += INC1;

	return LW_TASK_EXECUTE;
}

static int task2_handler(void *arg)
{
	struct execute_args *data = arg;

	data->data += INC2;

	return LW_TASK_EXECUTE;
}

static void interval_sleep(uint32_t num_intervals)
{

	/* k_sleep() adds an extra tick--hence the "-1" */

	k_sleep(K_TICKS((TICKS_PER_INTERVAL * num_intervals) - 1));
}

/* Set up the LW scheduler before each test */
void test_before(void *unused)
{
	struct lw_scheduler *sched;
	int  priority;

	ARG_UNUSED(unused);

	ops1.abort = NULL;
	args1.abort = NULL;
	abort_args1.called = false;

	ops2.abort = NULL;
	args2.abort = NULL;
	abort_args2.called = false;

	priority = k_thread_priority_get(k_current_get());

	sched = lw_scheduler_init(&test_sched, test_sched_stack,
				  K_THREAD_STACK_SIZEOF(sched_stack),
				  priority - 1, 0,
				  K_TICKS(TICKS_PER_INTERVAL));
	zassert_true(sched == &test_sched);

	/* Create two LW tasks */

	lw_task_init(&task1, &ops1, &args1, &test_sched, 42);
	lw_task_init(&task2, &ops2, &args2, &test_sched, 13);
}

ZTEST(lw_sched, test_lw_task_basic)
{
	uint32_t base1 = exe_args1.data;
	uint32_t base2 = exe_args2.data;
	uint32_t expect1;
	uint32_t expect2;

	lw_task_start(&task1);
	lw_task_start(&task2);

	interval_sleep(2);

	/* Verify lw tasks did not execute as lw scheduler has not started */

	expect1 = base1;
	expect2 = base2;
	zassert_true(expect1 == exe_args1.data);
	zassert_true(expect2 == exe_args2.data);

	lw_scheduler_start(&test_sched, K_NO_WAIT);
	interval_sleep(2);

	/* task1 and task2 are expected to each have executed twice. */

	expect1 = base1 + (2 * INC1);
	expect2 = base2 + (2 * INC2);
	zassert_true(exe_args1.data == expect1);
	zassert_true(exe_args2.data == expect2);

	base1 = expect1;
	base2 = expect2;
	lw_task_delay(&task1, 5);
	interval_sleep(10);

	/*
	 * test_sched expected to have executed 10 more times.
	 * task1 delayed for 5 and executed for 5.
	 * task2 executed for 10.
	 */

	expect1 = base1 + (5 * INC1);
	expect2 = base2 + (10 * INC2);
	zassert_true(exe_args1.data == expect1);
	zassert_true(exe_args2.data == expect2);

	base1 = expect1;
	base2 = expect2;
	lw_task_pause(&task1);   /* Pause task1 */
	interval_sleep(2);

	/* task1 should not have executed at all, task2 twice */

	expect1 = base1;
	expect2 = base2 + (2 * INC2);
	zassert_true(exe_args1.data == expect1);
	zassert_true(exe_args2.data == expect2);

	base1 = expect1;
	base2 = expect2;
	lw_task_start(&task1);    /* Resume task1 */
	lw_task_abort(&task2);    /* Abort task2 */
	interval_sleep(2);

	/* task1 should have executed twice, task2 not at all */

	expect1 = base1 + (2 * INC1);
	expect2 = base2;
	zassert_true(exe_args1.data == expect1);
	zassert_true(exe_args2.data == expect2);

	lw_scheduler_abort(&test_sched);
	interval_sleep(2);

	/* Neither task1 nor task2 should have executed */

	zassert_true(exe_args1.data == expect1);
	zassert_true(exe_args2.data == expect2);
}

ZTEST(lw_sched, test_lw_task_abort_handler)
{
	ops1.abort = abort_handler;
	args1.abort = &abort_args1;

	ops2.abort = abort_handler;
	args2.abort = &abort_args2;

	lw_scheduler_start(&test_sched, K_NO_WAIT);

	interval_sleep(5);

	lw_task_abort(&task1);
	zassert_true(abort_args1.called == false);

	lw_scheduler_abort(&test_sched);
	zassert_true(abort_args1.called == false);
	zassert_true(abort_args2.called == true);
}

ZTEST_SUITE(lw_sched, NULL, NULL, test_before, NULL, NULL);
