/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Offload to the Kernel workqueue
 *
 * This test verifies that the kernel workqueue operates as
 * expected.
 *
 * This test has two threads that increment a counter.  The routine that
 * increments the counter is invoked from workqueue due to the two threads
 * calling using it.  The final result of the counter is expected
 * to be the the number of times work item was called to increment
 * the counter.
 *
 * This is done with time slicing both disabled and enabled to ensure that the
 * result always matches the number of times the workqueue is called.
 *
 * @{
 * @}
 */
#include <zephyr.h>
#include <linker/sections.h>
#include <ztest.h>

#define NUM_MILLISECONDS        50
#define TEST_TIMEOUT            200

#ifdef CONFIG_COVERAGE
#define OFFLOAD_WORKQUEUE_STACK_SIZE 4096
#else
#define OFFLOAD_WORKQUEUE_STACK_SIZE 1024
#endif


static uint32_t critical_var;
static uint32_t alt_thread_iterations;

static struct k_work_q offload_work_q;
static K_THREAD_STACK_DEFINE(offload_work_q_stack,
			     OFFLOAD_WORKQUEUE_STACK_SIZE);

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACKSIZE)

static K_THREAD_STACK_DEFINE(stack1, STACK_SIZE);
static K_THREAD_STACK_DEFINE(stack2, STACK_SIZE);

static struct k_thread thread1;
static struct k_thread thread2;

K_SEM_DEFINE(ALT_SEM, 0, UINT_MAX);
K_SEM_DEFINE(REGRESS_SEM, 0, UINT_MAX);
K_SEM_DEFINE(TEST_SEM, 0, UINT_MAX);

/**
 * @brief Routine to be called from a workqueue
 *
 * This routine increments the global variable @a critical_var.
 */
void critical_rtn(struct k_work *unused)
{
	volatile uint32_t x;

	ARG_UNUSED(unused);

	x = critical_var;
	critical_var = x + 1;
}

/**
 * @brief Common code for invoking work
 *
 * @param tag text identifying the invocation context
 * @param count number of critical section calls made thus far
 *
 * @return number of critical section calls made by a thread
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
#if defined(CONFIG_ARCH_POSIX)
		k_busy_wait(50);
		/*
		 * For the POSIX arch this loop and critical_rtn would otherwise
		 * run in 0 time and therefore would never finish.
		 * => We purposely waste 50us per loop
		 */
#endif
	}
	TC_PRINT("End %s at %u\n", tag, (uint32_t)now);

	return count;
}

/**
 * @brief Alternate thread
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

/**
 * @brief Regression thread
 *
 * This routine invokes the workqueue many times. It also checks to
 * ensure that the number of times it is called matches the global variable
 * @a critical_var.
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
 * @brief Verify thread context
 *
 * @details Check whether variable value per-thread is saved
 * during context switch
 *
 * @ingroup kernel_workqueue_tests
 */
void test_offload_workqueue(void)
{
	critical_var = 0U;
	alt_thread_iterations = 0U;

	k_work_q_start(&offload_work_q,
		       offload_work_q_stack,
		       K_THREAD_STACK_SIZEOF(offload_work_q_stack),
		       CONFIG_MAIN_THREAD_PRIORITY);

	k_thread_create(&thread1, stack1, STACK_SIZE,
			alternate_thread, NULL, NULL, NULL,
			K_PRIO_PREEMPT(12), 0, K_NO_WAIT);

	k_thread_create(&thread2, stack2, STACK_SIZE,
			regression_thread, NULL, NULL, NULL,
			K_PRIO_PREEMPT(12), 0, K_NO_WAIT);

	zassert_true(k_sem_take(&TEST_SEM, K_MSEC(TEST_TIMEOUT * 2)) == 0,
		     "Timed out waiting for TEST_SEM");
}

void test_main(void)
{
	ztest_test_suite(kernel_offload_wq,
			 ztest_1cpu_unit_test(test_offload_workqueue)
			 );
	ztest_run_test_suite(kernel_offload_wq);
}
