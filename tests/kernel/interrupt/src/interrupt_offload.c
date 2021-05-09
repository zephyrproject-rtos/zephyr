/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <irq_offload.h>
#include <interrupt_util.h>

#define STACK_SIZE	1024
#define NUM_WORK	4

static struct k_work offload_work[NUM_WORK];
static struct k_work_q wq_queue;
static K_THREAD_STACK_DEFINE(wq_stack, STACK_SIZE);
static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static struct k_thread tdata;

static struct k_sem end_sem;

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

	k_sem_give(&end_sem);
}

/* offload work to work queue */
void isr_handler(const void *param)
{
	struct k_work *work = ((struct interrupt_param *)param)->work;
	int ret;

	zassert_not_null(work, "kwork should not be NULL");

	orig_t_keep_run = 0;

	ret = k_work_submit_to_queue(&wq_queue, work);
	zassert_true((ret == 0) || (ret == 1),
			"kwork not sumbmitted or queued");

}

#if defined(CONFIG_DYNAMIC_INTERRUPTS)
/*
 * So far, we only test x86 and arch posix by real dynamic interrupt.
 * Other arch will be add later.
 */
#if defined(CONFIG_X86)
#define IV_IRQS 32
#define TEST_IRQ_DYN_LINE 17
#define TRIGGER_IRQ_DYN_LINE (TEST_IRQ_DYN_LINE + IV_IRQS)

#elif defined(CONFIG_ARCH_POSIX)
#define TEST_IRQ_DYN_LINE 5
#define TRIGGER_IRQ_DYN_LINE 5

#else
#define TEST_IRQ_DYN_LINE -1
#define TRIGGER_IRQ_DYN_LINE -1
#endif

#endif

static void init_dyn_interrupt(void)
{
	/* If we cannot get a dynamic interrupt, skip test. */
	if (TEST_IRQ_DYN_LINE == -1) {
		ztest_test_skip();
	}

	/* We just initialize dynamic interrput once, then reuse them */
	if (!vector_num) {
		vector_num = irq_connect_dynamic(TEST_IRQ_DYN_LINE, 1,
					isr_handler, (void *)&irq_param, 0);
	}

	TC_PRINT("irq(%d)\n", vector_num);
	zassert_true(vector_num > 0, "no vector can be used");
	irq_enable(TEST_IRQ_DYN_LINE);
}

static void trigger_offload_interrupt(const bool real_irq, void *work)
{
	irq_param.work = work;

	if (real_irq) {
		trigger_irq(TRIGGER_IRQ_DYN_LINE);
	} else {
		irq_offload((irq_offload_routine_t)&isr_handler, &irq_param);
	}
}

static void t_running(void *p1, void *p2, void *p3)
{
	while (1) {
		orig_t_keep_run = 1;
		k_usleep(1);
	}
}

static void run_test_offload(int case_type, int real_irq)
{
	static bool wq_already_start;
	int thread_prio = 1;

	TC_PRINT("case %d\n", case_type);

	/* semaphore used to sync the end */
	k_sem_init(&end_sem, 0, NUM_WORK);

	if (offload_job_prio_higher) {
		/* priority of offload job higher than thread */
		thread_prio = 2;
	} else {
		/* priority of offload job lower than thread */
		thread_prio = 0;
	}

	if (real_irq && !vector_num) {
		init_dyn_interrupt();
	}

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)t_running,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(thread_prio),
			K_INHERIT_PERMS, K_NO_WAIT);

	/* start a work queue thread if not existing */
	if (!wq_already_start) {
		k_work_queue_start(&wq_queue, wq_stack, STACK_SIZE,
		1, NULL);

		wq_already_start = true;
	}

	/* initialize all the k_work */
	for (int i = 0; i < NUM_WORK; i++) {
		k_work_init(&offload_work[i], entry_offload_job);
	}

	/* wait for thread start */
	k_usleep(10);

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
	k_sem_take(&end_sem, K_FOREVER);

	k_usleep(1);

	zassert_equal(orig_t_keep_run, 1,
			"offload job done, the original thread run");

	k_thread_abort(tid);
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
void test_isr_offload_job_multiple(void)
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
void test_isr_offload_job_identi(void)
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
void test_isr_offload_job(void)
{
	if (!IS_ENABLED(CONFIG_DYNAMIC_INTERRUPTS)) {
		ztest_test_skip();
	}

	offload_job_prio_higher = true;
	run_test_offload(TEST_OFFLOAD_MULTI_JOBS, true);
	run_test_offload(TEST_OFFLOAD_IDENTICAL_JOBS, true);
}
