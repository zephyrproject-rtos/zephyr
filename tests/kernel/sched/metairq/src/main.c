/*
 * Copyright (c) 2019 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <ztest.h>
#include <kernel.h>

/*
 * Test that meta-IRQs return to the cooperative thread they preempted.
 *
 * A meta-IRQ thread unblocks first a long-running low-priority
 * cooperative thread, sleeps a little, and then unblocks a
 * high-priority cooperative thread before the low-priority thread has
 * finished. The correct behavior is to continue execution of the
 * low-priority thread and schedule the high-priority thread
 * afterwards.
 */

#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 1
#error Meta-IRQ test requires single-CPU operation
#endif

#if CONFIG_NUM_METAIRQ_PRIORITIES < 1
#error Need one metairq priority
#endif

#if CONFIG_NUM_COOP_PRIORITIES < 2
#error Need two cooperative priorities
#endif

K_SEM_DEFINE(metairq_sem, 0, 1);
K_SEM_DEFINE(coop_sem1, 0, 1);
K_SEM_DEFINE(coop_sem2, 0, 1);

/* Variables to track progress of cooperative threads */
volatile int coop_cnt1;
volatile int coop_cnt2;

#define WAIT_MS  10 /* Time to wait/sleep between actions */
#define LOOP_CNT  4 /* Number of times low priority thread waits */

/* Meta-IRQ thread */
void metairq_thread(void)
{
	k_sem_take(&metairq_sem, K_FOREVER);

	printk("metairq start\n");

	coop_cnt1 = 0;
	coop_cnt2 = 0;

	printk("give sem2\n");
	k_sem_give(&coop_sem2);

	k_msleep(WAIT_MS);

	printk("give sem1\n");
	k_sem_give(&coop_sem1);

	printk("metairq end, should switch back to co-op thread2\n");

	k_sem_give(&metairq_sem);
}

/* High-priority cooperative thread */
void coop_thread1(void)
{
	int cnt1, cnt2;

	printk("thread1 take sem\n");
	k_sem_take(&coop_sem1, K_FOREVER);
	printk("thread1 got sem\n");

	/* Expect that low-priority thread has run to completion */
	cnt1 = coop_cnt1;
	zassert_equal(cnt1, 0, "Unexpected cnt1 at start: %d", cnt1);
	cnt2 = coop_cnt2;
	zassert_equal(cnt2, LOOP_CNT, "Unexpected cnt2 at start: %d", cnt2);

	printk("thread1 increments coop_cnt1\n");
	coop_cnt1++;

	/* Expect that both threads have run to completion */
	cnt1 = coop_cnt1;
	zassert_equal(cnt1, 1, "Unexpected cnt1 at end: %d", cnt1);
	cnt2 = coop_cnt2;
	zassert_equal(cnt2, LOOP_CNT, "Unexpected cnt2 at end: %d", cnt2);

	k_sem_give(&coop_sem1);
}

/* Low-priority cooperative thread */
void coop_thread2(void)
{
	int cnt1, cnt2;

	printk("thread2 take sem\n");
	k_sem_take(&coop_sem2, K_FOREVER);
	printk("thread2 got sem\n");

	/* Expect that this is run first */
	cnt1 = coop_cnt1;
	zassert_equal(cnt1, 0, "Unexpected cnt1 at start: %d", cnt1);
	cnt2 = coop_cnt2;
	zassert_equal(cnt2, 0, "Unexpected cnt2 at start: %d", cnt2);

	/* At some point before this loop has finished, the meta-irq thread
	 * will have woken up and given the semaphore which thread1 was
	 * waiting on. It then exits. We need to ensure that this thread
	 * continues to run after that instead of scheduling thread 1
	 * when the meta-irq exits.
	 */
	for (int i = 0; i < LOOP_CNT; i++) {
		printk("thread2 loop iteration %d\n", i);
		coop_cnt2++;
		k_busy_wait(WAIT_MS * 1000);
	}

	/* Expect that this runs to completion before high-priority
	 * thread is started
	 */
	cnt1 = coop_cnt1;
	zassert_equal(cnt1, 0, "Unexpected cnt1 at end: %d", cnt1);
	cnt2 = coop_cnt2;
	zassert_equal(cnt2, LOOP_CNT, "Unexpected cnt2 at end: %d", cnt2);

	k_sem_give(&coop_sem2);
}

K_THREAD_DEFINE(metairq_thread_id, 1024,
		metairq_thread, 0, 0, 0,
		K_PRIO_COOP(0), 0, 0);
K_THREAD_DEFINE(coop_thread1_id, 1024,
		coop_thread1, 0, 0, 0,
		K_PRIO_COOP(1), 0, 0);
K_THREAD_DEFINE(coop_thread2_id, 1024,
		coop_thread2, 0, 0, 0,
		K_PRIO_COOP(2), 0, 0);

void test_preempt(void)
{
	/* Kick off meta-IRQ */
	k_sem_give(&metairq_sem);

	/* Wait for all threads to finish */
	k_sem_take(&coop_sem2, K_FOREVER);
	k_sem_take(&coop_sem1, K_FOREVER);
	k_sem_take(&metairq_sem, K_FOREVER);
}

void test_main(void)
{
	ztest_test_suite(suite_preempt,
			 ztest_unit_test(test_preempt));
	ztest_run_test_suite(suite_preempt);
}
