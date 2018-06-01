/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <irq_offload.h>
#include <misc/stack.h>

#define SLEEP_MS 100
#define NUM_OF_WORK 2
#define TEST_STRING "TEST"

static struct k_work work[NUM_OF_WORK];
static struct k_sem sync_sema;

#define STACKSIZE 512
K_THREAD_STACK_DEFINE(test_stack, STACKSIZE);
__kernel struct k_thread test_thread;

static int tcount;
static bool thread_flag;
static bool stack_flag;

/**TESTPOINT: stack analyze*/
static void tdata_dump_callback(const struct k_thread *thread, void *user_data)
{
	stack_analyze("Test", (char *)thread->stack_info.start,
						thread->stack_info.size);
}

/*power hook functions*/
int _sys_soc_suspend(s32_t ticks)
{
	static bool test_flag;

	/* Call k_thread_foreach only once otherwise it will
	 * flood the console with stack dumps.
	 */
	if (!test_flag) {
		k_thread_foreach(tdata_dump_callback, NULL);
		test_flag = true;
	}

	return 0;
}

void _sys_soc_resume(void)
{
}

/*work handler*/
static void work_handler(struct k_work *w)
{
	k_thread_foreach(tdata_dump_callback, NULL);
	k_sem_give(&sync_sema);
}

/*test cases*/
void test_call_stacks_analyze_main(void)
{
	TC_PRINT("from main thread:\n");
	k_thread_foreach(tdata_dump_callback, NULL);
}

void test_call_stacks_analyze_idle(void)
{
	TC_PRINT("from idle thread:\n");
	k_sleep(SLEEP_MS);
}

void test_call_stacks_analyze_workq(void)
{
	TC_PRINT("from workq:\n");
	k_sem_init(&sync_sema, 0, NUM_OF_WORK);
	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_work_init(&work[i], work_handler);
		k_work_submit(&work[i]);
		k_sem_take(&sync_sema, K_FOREVER);
	}
}

static void thread_entry(void *p1, void *p2, void *p3)
{
	k_sleep(SLEEP_MS);
}

static void thread_callback(const struct k_thread *thread, void *user_data)
{
	char *str = (char *)user_data;

	if (thread == &test_thread) {
		TC_PRINT("%s: Newly added thread found\n", str);
		TC_PRINT("%s: tid: %p, prio: %d\n",
				str, thread, thread->base.prio);
		thread_flag = true;
	}

	if ((char *)thread->stack_info.start ==
			K_THREAD_STACK_BUFFER(test_stack)) {
		TC_PRINT("%s: Newly added thread stack found\n", str);
		TC_PRINT("%s: stack:%p, size:%u\n", str,
					(char *)thread->stack_info.start,
					thread->stack_info.size);
		stack_flag = true;
	}

	tcount++;
}

static void test_k_thread_foreach(void)
{
	int count;

	/* Call k_thread_foreach() and check
	 * thread_callback is getting called.
	 */
	k_thread_foreach(thread_callback, TEST_STRING);

	/* Check thread_count non-zero, thread_flag
	 * and stack_flag are not set.
	 */
	zassert_true(tcount && !thread_flag && !stack_flag,
				"thread_callback() not getting called");
	/* Save the initial thread count */
	count = tcount;

	/* Create new thread which should add a new entry to the thread list */
	k_tid_t tid = k_thread_create(&test_thread, test_stack,
			STACKSIZE, (k_thread_entry_t)thread_entry, NULL,
			NULL, NULL, K_PRIO_PREEMPT(0), 0, 0);
	k_sleep(1);

	/* Call k_thread_foreach() and check
	 * thread_callback is getting called for
	 * the newly added thread.
	 */
	tcount = 0;
	k_thread_foreach(thread_callback, TEST_STRING);

	/* Check thread_count > temp, thread_flag and stack_flag are set */
	zassert_true((tcount > count) && thread_flag && stack_flag,
					"thread_callback() not getting called");
	k_thread_abort(tid);
}

/*TODO: add test case to capture the usage of interrupt call stack*/


void test_main(void)
{
	ztest_test_suite(profiling_api,
			 ztest_unit_test(test_call_stacks_analyze_main),
			 ztest_unit_test(test_call_stacks_analyze_idle),
			 ztest_unit_test(test_call_stacks_analyze_workq),
			 ztest_unit_test(test_k_thread_foreach));
	ztest_run_test_suite(profiling_api);
}
