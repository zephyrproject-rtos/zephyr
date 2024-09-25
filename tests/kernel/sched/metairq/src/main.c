/*
 * Copyright (c) 2019 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

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

#if defined(CONFIG_SMP) && CONFIG_MP_MAX_NUM_CPUS > 1
#error Meta-IRQ test requires single-CPU operation
#endif

#if CONFIG_NUM_METAIRQ_PRIORITIES < 1
#error Need one metairq priority
#endif

#if CONFIG_NUM_COOP_PRIORITIES < 2
#error Need two cooperative priorities
#endif


#define STACKSIZE 1024
#define DEFINE_PARTICIPANT_THREAD(id)                                       \
		K_THREAD_STACK_DEFINE(thread_##id##_stack_area, STACKSIZE); \
		struct k_thread thread_##id##_thread_data;                  \
		k_tid_t thread_##id##_tid;

#define PARTICIPANT_THREAD_OPTIONS (0)
#define CREATE_PARTICIPANT_THREAD(id, pri, entry)                                      \
		k_thread_create(&thread_##id##_thread_data, thread_##id##_stack_area,  \
			K_THREAD_STACK_SIZEOF(thread_##id##_stack_area),               \
			entry,                                                         \
			NULL, NULL, NULL,                                              \
			pri, PARTICIPANT_THREAD_OPTIONS, K_FOREVER);
#define START_PARTICIPANT_THREAD(id) k_thread_start(&(thread_##id##_thread_data));
#define JOIN_PARTICIPANT_THREAD(id) k_thread_join(&(thread_##id##_thread_data), K_FOREVER);


K_SEM_DEFINE(metairq_sem, 0, 1);
K_SEM_DEFINE(coop_sem1, 0, 1);
K_SEM_DEFINE(coop_sem2, 0, 1);

/* Variables to track progress of cooperative threads */
volatile int coop_cnt1;
volatile int coop_cnt2;

#define WAIT_MS  10 /* Time to wait/sleep between actions */
#define LOOP_CNT  4 /* Number of times low priority thread waits */

/* Meta-IRQ thread */
void metairq_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

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
void coop_thread1(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

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
void coop_thread2(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

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

DEFINE_PARTICIPANT_THREAD(metairq_thread_id);
DEFINE_PARTICIPANT_THREAD(coop_thread1_id);
DEFINE_PARTICIPANT_THREAD(coop_thread2_id);

void create_participant_threads(void)
{
	CREATE_PARTICIPANT_THREAD(metairq_thread_id, K_PRIO_COOP(0), metairq_thread);
	CREATE_PARTICIPANT_THREAD(coop_thread1_id, K_PRIO_COOP(1), coop_thread1);
	CREATE_PARTICIPANT_THREAD(coop_thread2_id, K_PRIO_COOP(2), coop_thread2);
}

void start_participant_threads(void)
{
	START_PARTICIPANT_THREAD(metairq_thread_id);
	START_PARTICIPANT_THREAD(coop_thread1_id);
	START_PARTICIPANT_THREAD(coop_thread2_id);
}

void join_participant_threads(void)
{
	JOIN_PARTICIPANT_THREAD(metairq_thread_id);
	JOIN_PARTICIPANT_THREAD(coop_thread1_id);
	JOIN_PARTICIPANT_THREAD(coop_thread2_id);
}

ZTEST(suite_preempt_metairq, test_preempt_metairq)
{
	create_participant_threads();
	start_participant_threads();

	/* This unit test function runs on the ztest thread when
	 * CONFIG_MULTITHREADING=y.
	 * The ztest thread has a priority of CONFIG_ZTEST_THREAD_PRIORITY=-1.
	 * So it is cooperative, which cannot be preempted by the coop_thread1
	 * and coop_thread2 created and started above.
	 * This test requires coop_thread1/2 to wait on coop_sem1/2 before the
	 * metairq thread starts.
	 * Below sleep ensures the ztest thread relinquish the cpu and give
	 * coop_thread1/2 a chance to run and wait on coop_sem1/2.
	 */
	k_msleep(10);

	/* Kick off meta-IRQ */
	k_sem_give(&metairq_sem);

	/* Wait for all threads to finish */
	k_sem_take(&coop_sem2, K_FOREVER);
	k_sem_take(&coop_sem1, K_FOREVER);
	k_sem_take(&metairq_sem, K_FOREVER);

	join_participant_threads();
}

ZTEST_SUITE(suite_preempt_metairq, NULL, NULL, NULL, NULL, NULL);
