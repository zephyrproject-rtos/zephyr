/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/irq_offload.h>
#include <interrupt_util.h>

#define STACK_SIZE	1024
#define NUM_WORK	4

static struct k_work offload_work[NUM_WORK];
static struct k_work_q wq_queue;
static K_THREAD_STACK_DEFINE(wq_stack, STACK_SIZE);
static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static struct k_thread tdata;

static struct k_sem sync_sem;
static struct k_sem end_sem;
static bool wait_for_end;
static atomic_t submit_success;
static atomic_t offload_job_cnt;

/*
 * This global variable control if the priority of offload job
 * is greater than the original thread.
 */
static bool offload_job_prio_higher;

static volatile int orig_t_keep_run;

/* record the initialized interrupt vector for reuse */
static int vector_num;

enum {
	TEST_OFFLOAD_MULTI_JOBS,
	TEST_OFFLOAD_IDENTICAL_JOBS
};

struct interrupt_param {
	struct k_work *work;
};

static struct interrupt_param irq_param;

/* thread entry of doing offload job */
static void entry_offload_job(struct k_work *work)
{
	if (offload_job_prio_higher) {
		/*TESTPOINT: offload thread run right after irq end.*/
		zassert_equal(orig_t_keep_run, 0,
			"the offload did not run immediately.");
	} else {
		/*TESTPOINT: original thread run right after irq end.*/
		zassert_equal(orig_t_keep_run, 1,
			"the offload did not run immediately.");
	}

	atomic_inc(&offload_job_cnt);
	k_sem_give(&end_sem);
}

/* offload work to work queue */
void isr_handler(const void *param)
{
	struct k_work *work = ((struct interrupt_param *)param)->work;

	zassert_not_null(work, "kwork should not be NULL");

	orig_t_keep_run = 0;

	/* If the work is busy, we don't submit it. */
	if (!k_work_busy_get(work)) {
		zassert_equal(k_work_submit_to_queue(&wq_queue, work),
				1, "kwork not submitted or queued");

		atomic_inc(&submit_success);
	}
}

#if defined(CONFIG_DYNAMIC_INTERRUPTS)
/*
 * So far, we only test x86 and arch posix by real dynamic interrupt.
 * Other arch will be add later.
 */
#if defined(CONFIG_X86)
#define TEST_IRQ_DYN_LINE 26

#elif defined(CONFIG_ARCH_POSIX)
#define TEST_IRQ_DYN_LINE 5

#else
#define TEST_IRQ_DYN_LINE 0
#endif

#endif

static void init_dyn_interrupt(void)
{
	/* If we cannot get a dynamic interrupt, skip test. */
	if (TEST_IRQ_DYN_LINE == 0) {
		ztest_test_skip();
	}

	/* We just initialize dynamic interrupt once, then reuse them */
	if (!vector_num) {
		vector_num = irq_connect_dynamic(TEST_IRQ_DYN_LINE, 1,
					isr_handler, (void *)&irq_param, 0);
	}

	TC_PRINT("vector(%d)\n", vector_num);
	zassert_true(vector_num > 0, "no vector can be used");
	irq_enable(TEST_IRQ_DYN_LINE);
}

static void trigger_offload_interrupt(const bool real_irq, void *work)
{
	irq_param.work = work;

	if (real_irq) {
		trigger_irq(vector_num);
	} else {
		irq_offload((irq_offload_routine_t)&isr_handler, &irq_param);
	}
}

static void t_running(void *p1, void *p2, void *p3)
{
	k_sem_give(&sync_sem);

	while (wait_for_end == false) {
		orig_t_keep_run = 1;
		k_usleep(150);
	}
}

static void init_env(int real_irq)
{
	static bool wq_already_start;

	/* semaphore used to sync the end */
	k_sem_init(&sync_sem, 0, 1);
	k_sem_init(&end_sem, 0, NUM_WORK);

	/* initialize global variables */
	submit_success = 0;
	offload_job_cnt = 0;
	orig_t_keep_run = 0;
	wait_for_end = false;

	/* initialize the dynamic interrupt while using it */
	if (real_irq && !vector_num) {
		init_dyn_interrupt();
	}

	/* initialize all the k_work */
	for (int i = 0; i < NUM_WORK; i++) {
		k_work_init(&offload_work[i], entry_offload_job);
	}

	/* start a work queue thread if not existing */
	if (!wq_already_start) {
		k_work_queue_start(&wq_queue, wq_stack, STACK_SIZE,
		K_PRIO_PREEMPT(1), NULL);

		wq_already_start = true;
	}
}

static void run_test_offload(int case_type, int real_irq)
{
	int thread_prio = K_PRIO_PREEMPT(0);

	/* initialize the global variables */
	init_env(real_irq);

	/* set priority of offload job higher than thread */
	if (offload_job_prio_higher) {
		thread_prio = K_PRIO_PREEMPT(2);
	}

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)t_running,
			NULL, NULL, NULL, thread_prio,
			K_INHERIT_PERMS, K_NO_WAIT);

	/* wait for thread start */
	k_sem_take(&sync_sem, K_FOREVER);

	for (int i = 0; i < NUM_WORK; i++) {

		switch (case_type) {
		case TEST_OFFLOAD_MULTI_JOBS:
			trigger_offload_interrupt(real_irq,
					(void *)&offload_work[i]);
		break;
		case TEST_OFFLOAD_IDENTICAL_JOBS:
			trigger_offload_interrupt(real_irq,
					(void *)&offload_work[0]);
		break;
		default:
			ztest_test_fail();
		}
	}
	/* wait for all offload job complete */
	for (int i = 0; i < atomic_get(&submit_success); i++) {
		k_sem_take(&end_sem, K_FOREVER);
	}

	zassert_equal(submit_success, offload_job_cnt,
			"submitted job unmatch offload");

	/* notify the running thread to end */
	wait_for_end = true;

	k_thread_join(tid, K_FOREVER);
}

/**
 * @brief Test interrupt offload work to multiple jobs
 *
 * @ingroup kernel_interrupt_tests
 *
 * @details Validate isr can offload workload to multi work queue, and:
 *
 * - If the priority of the original thread < offload job, offload jobs
 *   could execute immediately.
 *
 * - If the priority of the original thread >= offload job, offload
 *   jobs will not execute immediately.
 *
 * We test this by irq_offload().
 */
ZTEST(interrupt_feature, test_isr_offload_job_multiple)
{
	offload_job_prio_higher = false;
	run_test_offload(TEST_OFFLOAD_MULTI_JOBS, false);

	offload_job_prio_higher = true;
	run_test_offload(TEST_OFFLOAD_MULTI_JOBS, false);
}

/**
 * @brief Test interrupt offload work to identical jobs
 *
 * @ingroup kernel_interrupt_tests
 *
 * @details Validate isr can offload workload to work queue, and all
 * the offload jobs use the same thread entry, and:
 *
 * - If the priority of the original thread < offload job, offload jobs
 *   could execute immediately.
 *
 * - If the priority of the original thread >= offload job, offload
 *   jobs will not execute immediately.
 *
 * We test this by irq_offload().
 */
ZTEST(interrupt_feature, test_isr_offload_job_identi)
{
	offload_job_prio_higher = false;
	run_test_offload(TEST_OFFLOAD_IDENTICAL_JOBS, false);

	offload_job_prio_higher = true;
	run_test_offload(TEST_OFFLOAD_IDENTICAL_JOBS, false);
}

/**
 * @brief Test interrupt offload work by dynamic interrupt
 *
 * @ingroup kernel_interrupt_tests
 *
 * @details Validate isr can offload workload to work queue, and the
 * offload jobs could execute immediately base on it's priority.
 * We test this by dynamic interrupt.
 */
ZTEST(interrupt_feature, test_isr_offload_job)
{
	if (!IS_ENABLED(CONFIG_DYNAMIC_INTERRUPTS)) {
		ztest_test_skip();
	}

	offload_job_prio_higher = true;
	run_test_offload(TEST_OFFLOAD_MULTI_JOBS, true);
	run_test_offload(TEST_OFFLOAD_IDENTICAL_JOBS, true);
}
