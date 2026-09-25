/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Offload critical sections to a workqueue
 *
 * This test verifies that work offloaded to a workqueue by concurrently
 * running threads is serialized by the workqueue thread, so it can be used
 * as a critical section.
 *
 * This test has two threads that increment a counter.  The routine that
 * increments the counter is invoked from workqueue due to the two threads
 * calling using it.  The final result of the counter is expected
 * to be the number of times work item was called to increment
 * the counter.
 *
 * This is done with time slicing both disabled and enabled to ensure that the
 * result always matches the number of times the workqueue is called.
 */
#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>
#include <zephyr/ztest.h>

#define NUM_MILLISECONDS        50
#define TEST_TIMEOUT            200

#ifdef CONFIG_COVERAGE_GCOV
#define OFFLOAD_WORKQUEUE_STACK_SIZE 4096
#else
#define OFFLOAD_WORKQUEUE_STACK_SIZE 1024
#endif


static uint32_t critical_var;
static uint32_t alt_thread_iterations;

static struct k_work_q offload_work_q;
static K_THREAD_STACK_DEFINE(offload_work_q_stack,
			     OFFLOAD_WORKQUEUE_STACK_SIZE);

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

static K_THREAD_STACK_DEFINE(stack1, STACK_SIZE);
static K_THREAD_STACK_DEFINE(stack2, STACK_SIZE);

static struct k_thread thread1;
static struct k_thread thread2;

K_SEM_DEFINE(ALT_SEM, 0, UINT_MAX);
K_SEM_DEFINE(REGRESS_SEM, 0, UINT_MAX);
K_SEM_DEFINE(TEST_SEM, 0, UINT_MAX);

/*
 * Routine to be called from a workqueue
 *
 * This routine increments the global variable critical_var.
 */
void critical_rtn(struct k_work *unused)
{
	volatile uint32_t x;

	ARG_UNUSED(unused);

	x = critical_var;
	critical_var = x + 1;
}

/*
 * Common code for invoking work
 *
 * tag: text identifying the invocation context
 * count: number of critical section calls made thus far
 *
 * Returns number of critical section calls made by a thread
 */
uint32_t critical_loop(const char *tag, uint32_t count)
{
	int64_t now;
	int64_t last;
	int64_t mseconds;

	last = mseconds = k_uptime_get();
	TC_PRINT("Start %s at %u\n", tag, (uint32_t)last);
	while (((now = k_uptime_get())) < mseconds + NUM_MILLISECONDS) {
		struct k_work work_item;

		if (now < last) {
			TC_PRINT("Time went backwards: %u < %u\n",
				 (uint32_t)now, (uint32_t)last);
		}
		last = now;

		k_work_init(&work_item, critical_rtn);
		k_work_submit_to_queue(&offload_work_q, &work_item);
		count++;
		Z_SPIN_DELAY(50);
	}
	TC_PRINT("End %s at %u\n", tag, (uint32_t)now);

	return count;
}

/*
 * Alternate thread
 *
 * This routine invokes the workqueue many times.
 */
void alternate_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	k_sem_take(&ALT_SEM, K_FOREVER);        /* Wait to be activated */

	alt_thread_iterations = critical_loop("alt1", alt_thread_iterations);

	k_sem_give(&REGRESS_SEM);

	k_sem_take(&ALT_SEM, K_FOREVER);        /* Wait to be re-activated */

	alt_thread_iterations = critical_loop("alt2", alt_thread_iterations);

	k_sem_give(&REGRESS_SEM);
}

/*
 * Regression thread
 *
 * This routine invokes the workqueue many times. It also checks to
 * ensure that the number of times it is called matches the global variable
 * critical_var.
 */

void regression_thread(void *arg1, void *arg2, void *arg3)
{
	uint32_t ncalls = 0U;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	k_sem_give(&ALT_SEM);   /* Activate alternate_thread() */

	ncalls = critical_loop("reg1", ncalls);

	/* Wait for alternate_thread() to complete */
	zassert_true(k_sem_take(&REGRESS_SEM, K_MSEC(TEST_TIMEOUT)) == 0,
		     "Timed out waiting for REGRESS_SEM");

	zassert_equal(critical_var, ncalls + alt_thread_iterations,
		      "Unexpected value for <critical_var>");

	TC_PRINT("Enable timeslicing at %u\n", k_uptime_get_32());
	k_sched_time_slice_set(20, 10);

	k_sem_give(&ALT_SEM);   /* Re-activate alternate_thread() */

	ncalls = critical_loop("reg2", ncalls);

	/* Wait for alternate_thread() to finish */
	zassert_true(k_sem_take(&REGRESS_SEM, K_MSEC(TEST_TIMEOUT)) == 0,
		     "Timed out waiting for REGRESS_SEM");

	zassert_equal(critical_var, ncalls + alt_thread_iterations,
		      "Unexpected value for <critical_var>");

	k_sem_give(&TEST_SEM);

}

/**
 * @brief Verify that a workqueue serializes work offloaded by competing threads.
 *
 * @details
 * A dedicated workqueue is used as a critical section: two preemptible threads
 * of equal priority repeatedly submit a work item that performs a
 * read-modify-write on a shared counter. Because the workqueue thread runs the
 * submitted items one at a time, no update may be lost, and the counter must
 * end up equal to the total number of work items submitted by both threads.
 * The sequence is run twice, first with time slicing disabled and then with it
 * enabled, so that preemption between the two submitting threads is exercised
 * in both scheduling modes.
 *
 * Test steps:
 * - Start a dedicated workqueue with k_work_queue_start().
 * - Create an alternate and a regression thread at the same preemptible
 *   priority, each submitting a counter-incrementing work item to that queue
 *   in a loop for a fixed period.
 * - Once both threads finish, compare the shared counter against the number of
 *   work items each thread submitted.
 * - Enable time slicing with k_sched_time_slice_set() and repeat the loop and
 *   the comparison.
 *
 * Expected result:
 * - After each round the shared counter equals the total number of work items
 *   submitted by both threads, with time slicing both disabled and enabled.
 *
 * @ingroup kernel_workqueue_tests
 * @see k_work_queue_start()
 * @see k_work_init()
 * @see k_work_submit_to_queue()
 * @see k_sched_time_slice_set()
 */
ZTEST(kernel_offload_wq, test_workq_offload_critical_section)
{
	critical_var = 0U;
	alt_thread_iterations = 0U;

	k_work_queue_start(&offload_work_q,
		       offload_work_q_stack,
		       K_THREAD_STACK_SIZEOF(offload_work_q_stack),
		       CONFIG_MAIN_THREAD_PRIORITY, NULL);

	k_thread_create(&thread1, stack1, STACK_SIZE,
			alternate_thread, NULL, NULL, NULL,
			K_PRIO_PREEMPT(12), 0, K_NO_WAIT);

	k_thread_create(&thread2, stack2, STACK_SIZE,
			regression_thread, NULL, NULL, NULL,
			K_PRIO_PREEMPT(12), 0, K_NO_WAIT);

	zassert_true(k_sem_take(&TEST_SEM, K_MSEC(TEST_TIMEOUT * 2)) == 0,
		     "Timed out waiting for TEST_SEM");
}

ZTEST_SUITE(kernel_offload_wq, NULL, NULL, ztest_simple_1cpu_before,
		 ztest_simple_1cpu_after, NULL);
