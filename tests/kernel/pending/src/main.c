/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <stdbool.h>

#define  NUM_SECONDS(x)      ((x) * 1000)
#define  HALF_SECOND                (500)
#define  THIRD_SECOND               (333)
#define  FOURTH_SECOND              (250)

#define COOP_STACKSIZE   (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define PREEM_STACKSIZE  (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

#define FIFO_TEST_START       10
#define FIFO_TEST_END         20

#define SEM_TEST_START        30
#define SEM_TEST_END          40

#define LIFO_TEST_START       50
#define LIFO_TEST_END         60

#define NON_NULL_PTR          ((void *)0x12345678)

#ifdef CONFIG_COVERAGE
#define OFFLOAD_WORKQUEUE_STACK_SIZE 4096
#else
#define OFFLOAD_WORKQUEUE_STACK_SIZE 1024
#endif

#define OFFLOAD_WORKQUEUE_PRIORITY	(-1)
static struct k_work_q offload_work_q;
static K_THREAD_STACK_DEFINE(offload_work_q_stack,
			     OFFLOAD_WORKQUEUE_STACK_SIZE);

struct fifo_data {
	intptr_t reserved;
	uint32_t data;
};

struct lifo_data {
	intptr_t reserved;
	uint32_t data;
};

struct offload_work {
	struct k_work work_item;
	struct k_sem *sem;
};

static K_THREAD_STACK_ARRAY_DEFINE(coop_stack, 2, COOP_STACKSIZE);
static struct k_thread coop_thread[2];

static struct k_fifo fifo;
static struct k_lifo lifo;
static struct k_timer timer;

static struct k_sem start_test_sem;
static struct k_sem sync_test_sem;
static struct k_sem end_test_sem;

struct fifo_data fifo_test_data[4] = {
	{ 0, FIFO_TEST_END + 1 }, { 0, FIFO_TEST_END + 2 },
	{ 0, FIFO_TEST_END + 3 }, { 0, FIFO_TEST_END + 4 }
};

struct lifo_data lifo_test_data[4] = {
	{ 0, LIFO_TEST_END + 1 }, { 0, LIFO_TEST_END + 2 },
	{ 0, LIFO_TEST_END + 3 }, { 0, LIFO_TEST_END + 4 }
};

static uint32_t timer_start_tick;
static uint32_t timer_end_tick;
static void *timer_data;

static int __noinit coop_high_state;
static int __noinit coop_low_state;
static int __noinit task_high_state;
static int __noinit task_low_state;

static int __noinit counter;

static inline void *my_fifo_get(struct k_fifo *fifo, int32_t timeout)
{
	return k_fifo_get(fifo, K_MSEC(timeout));
}

static inline void *my_lifo_get(struct k_lifo *lifo, int32_t timeout)
{
	return k_lifo_get(lifo, K_MSEC(timeout));
}

static int increment_counter(void)
{
	int tmp;
	unsigned int key = irq_lock();

	tmp = ++counter;
	irq_unlock(key);

	return tmp;
}

static void sync_threads(struct k_work *work)
{
	struct offload_work *offload =
		CONTAINER_OF(work, struct offload_work, work_item);

	k_sem_give(offload->sem);
	k_sem_give(offload->sem);
	k_sem_give(offload->sem);
	k_sem_give(offload->sem);

}

static void fifo_tests(int32_t timeout, volatile int *state,
		       void *(*get)(struct k_fifo *, int32_t),
		       int (*sem_take)(struct k_sem *, k_timeout_t))
{
	struct fifo_data *data;

	sem_take(&start_test_sem, K_FOREVER);

	*state = FIFO_TEST_START;
	/* Expect this to time out */
	data = get(&fifo, timeout);
	if (data != NULL) {
		TC_ERROR("**** Unexpected data on FIFO get\n");
		return;
	}
	*state = increment_counter();

	/* Sync up fifo test threads */
	sem_take(&sync_test_sem, K_FOREVER);

	/* Expect this to receive data from the fifo */
	*state = FIFO_TEST_END;
	data = get(&fifo, timeout);
	if (data == NULL) {
		TC_ERROR("**** No data on FIFO get\n");
		return;
	}
	*state = increment_counter();

	if (data->data != *state) {
		TC_ERROR("**** Got FIFO data %d, not %d (%d)\n",
			 data->data, *state, timeout);
		return;
	}

	sem_take(&end_test_sem, K_FOREVER);
}

static void lifo_tests(int32_t timeout, volatile int *state,
		       void *(*get)(struct k_lifo *, int32_t),
		       int (*sem_take)(struct k_sem *, k_timeout_t))
{
	struct lifo_data *data;

	sem_take(&start_test_sem, K_FOREVER);

	*state = LIFO_TEST_START;
	/* Expect this to time out */
	data = get(&lifo, timeout);
	if (data != NULL) {
		TC_ERROR("**** Unexpected data on LIFO get\n");
		return;
	}
	*state = increment_counter();

	/* Sync up all threads */
	sem_take(&sync_test_sem, K_FOREVER);

	/* Expect this to receive data from the lifo */
	*state = LIFO_TEST_END;
	data = get(&lifo, timeout);
	if (data == NULL) {
		TC_ERROR("**** No data on LIFO get\n");
		return;
	}
	*state = increment_counter();

	if (data->data != *state) {
		TC_ERROR("**** Got LIFO data %d, not %d (%d)\n",
			 data->data, *state, timeout);
		return;
	}

	sem_take(&end_test_sem, K_FOREVER);
}

static void timer_tests(void)
{
	k_sem_take(&start_test_sem, K_FOREVER);

	timer_start_tick = k_uptime_get_32();

	k_timer_start(&timer, K_SECONDS(1), K_NO_WAIT);

	if (k_timer_status_sync(&timer)) {
		timer_data = timer.user_data;
	}

	timer_end_tick = k_uptime_get_32();

	k_sem_take(&end_test_sem, K_FOREVER);
}

static void coop_high(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	fifo_tests(NUM_SECONDS(1), &coop_high_state, my_fifo_get, k_sem_take);

	lifo_tests(NUM_SECONDS(1), &coop_high_state, my_lifo_get, k_sem_take);
}

static void coop_low(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	fifo_tests(HALF_SECOND, &coop_low_state, my_fifo_get, k_sem_take);

	lifo_tests(HALF_SECOND, &coop_low_state, my_lifo_get, k_sem_take);
}

void task_high(void)
{
	k_fifo_init(&fifo);
	k_lifo_init(&lifo);

	k_timer_init(&timer, NULL, NULL);
	timer.user_data = NON_NULL_PTR;

	k_sem_init(&start_test_sem, 0, UINT_MAX);
	k_sem_init(&sync_test_sem, 0, UINT_MAX);
	k_sem_init(&end_test_sem, 0, UINT_MAX);

	k_work_queue_start(&offload_work_q,
		       offload_work_q_stack,
		       K_THREAD_STACK_SIZEOF(offload_work_q_stack),
		       OFFLOAD_WORKQUEUE_PRIORITY, NULL);

	counter = SEM_TEST_START;

	k_thread_create(&coop_thread[0], coop_stack[0], COOP_STACKSIZE,
			coop_high, NULL, NULL, NULL, K_PRIO_COOP(3), 0,
			K_NO_WAIT);

	k_thread_create(&coop_thread[1], coop_stack[1], COOP_STACKSIZE,
			coop_low, NULL, NULL, NULL, K_PRIO_COOP(7), 0,
			K_NO_WAIT);

	counter = FIFO_TEST_START;
	fifo_tests(THIRD_SECOND, &task_high_state, my_fifo_get, k_sem_take);

	counter = LIFO_TEST_START;
	lifo_tests(THIRD_SECOND, &task_high_state, my_lifo_get, k_sem_take);

	timer_tests();
}

void task_low(void)
{
	fifo_tests(FOURTH_SECOND, &task_low_state, my_fifo_get, k_sem_take);

	lifo_tests(FOURTH_SECOND, &task_low_state, my_lifo_get, k_sem_take);
}

/**
 * @brief Test pending
 *
 * @defgroup kernel_pending_tests Pending tests
 *
 * @ingroup all_tests
 *
 * @{
 */

/**
 * @brief Test pending of workq, fifo and lifo
 *
 * @see k_sleep(), K_THREAD_DEFINE()
 */
ZTEST(pending, test_pending_fifo)
{
	/*
	 * Main thread(test_main) priority was 9 but ztest thread runs at
	 * priority -1. To run the test smoothly make both main and ztest
	 * threads run at same priority level.
	 */
	k_thread_priority_set(k_current_get(), 9);

	struct offload_work offload1 = {0};

	k_work_init(&offload1.work_item, sync_threads);
	offload1.sem = &start_test_sem;
	k_work_submit_to_queue(&offload_work_q, &offload1.work_item);

	/*
	 * Verify that preemptible threads 'task_high' and 'task_low' do not
	 * busy-wait. If they are not busy-waiting, then they must be pending.
	 */

	TC_PRINT("Testing preemptible threads block on fifos ...\n");
	zassert_false((coop_high_state != FIFO_TEST_START) ||
		      (coop_low_state != FIFO_TEST_START) ||
		      (task_high_state != FIFO_TEST_START) ||
		      (task_low_state != FIFO_TEST_START), NULL);

	/* Give waiting threads time to time-out */
	k_sleep(K_SECONDS(2));

	/*
	 * Verify that the cooperative and preemptible threads timed-out in
	 * the correct order.
	 */

	TC_PRINT("Testing fifos time-out in correct order ...\n");
	zassert_false((task_low_state != FIFO_TEST_START + 1) ||
		      (task_high_state != FIFO_TEST_START + 2) ||
		      (coop_low_state != FIFO_TEST_START + 3) ||
		      (coop_high_state != FIFO_TEST_START + 4),
		      "**** Threads timed-out in unexpected order");

	counter = FIFO_TEST_END;

	k_work_init(&offload1.work_item, sync_threads);
	offload1.sem = &sync_test_sem;
	k_work_submit_to_queue(&offload_work_q, &offload1.work_item);

	/*
	 * Two cooperative and two preemptible threads should be waiting on
	 * the FIFO
	 */

	/* Add data to the FIFO */
	TC_PRINT("Testing  fifos delivered data correctly ...\n");
	k_fifo_put(&fifo, &fifo_test_data[0]);
	k_fifo_put(&fifo, &fifo_test_data[1]);
	k_fifo_put(&fifo, &fifo_test_data[2]);
	k_fifo_put(&fifo, &fifo_test_data[3]);

	zassert_false((coop_high_state != FIFO_TEST_END + 1) ||
		      (coop_low_state != FIFO_TEST_END + 2) ||
		      (task_high_state != FIFO_TEST_END + 3) ||
		      (task_low_state != FIFO_TEST_END + 4),
		      "**** Unexpected delivery order");
}


ZTEST(pending, test_pending_lifo)
{
	/*
	 * Main thread(test_main) priority was 9 but ztest thread runs at
	 * priority -1. To run the test smoothly make both main and ztest
	 * threads run at same priority level.
	 */
	k_thread_priority_set(k_current_get(), 9);

	struct offload_work offload1 = {0};
	struct offload_work offload2 = {0};

	k_work_init(&offload1.work_item, sync_threads);
	offload1.sem = &end_test_sem;
	k_work_submit_to_queue(&offload_work_q, &offload1.work_item);

	k_work_init(&offload2.work_item, sync_threads);
	offload2.sem = &start_test_sem;
	k_work_submit_to_queue(&offload_work_q, &offload2.work_item);

	/*
	 * Verify that cooperative threads 'task_high' and 'task_low' do not
	 * busy-wait. If they are not busy-waiting, then they must be pending.
	 */

	TC_PRINT("Testing preemptible threads block on lifos ...\n");
	zassert_false((coop_high_state != LIFO_TEST_START) ||
		      (coop_low_state != LIFO_TEST_START) ||
		      (task_high_state != LIFO_TEST_START) ||
		      (task_low_state != LIFO_TEST_START), NULL);

	/* Give waiting threads time to time-out */
	k_sleep(K_SECONDS(2));

	TC_PRINT("Testing lifos time-out in correct order ...\n");
	zassert_false((task_low_state != LIFO_TEST_START + 1) ||
		      (task_high_state != LIFO_TEST_START + 2) ||
		      (coop_low_state != LIFO_TEST_START + 3) ||
		      (coop_high_state != LIFO_TEST_START + 4),
		      "**** Threads timed-out in unexpected order");

	counter = LIFO_TEST_END;

	k_work_init(&offload1.work_item, sync_threads);
	offload1.sem = &sync_test_sem;
	k_work_submit_to_queue(&offload_work_q, &offload1.work_item);

	/*
	 * Two cooperative threads and two preemptive threads should
	 * be waiting on the LIFO
	 */

	/* Add data to the LIFO */
	k_lifo_put(&lifo, &lifo_test_data[0]);
	k_lifo_put(&lifo, &lifo_test_data[1]);
	k_lifo_put(&lifo, &lifo_test_data[2]);
	k_lifo_put(&lifo, &lifo_test_data[3]);

	TC_PRINT("Testing lifos delivered data correctly ...\n");
	zassert_false((coop_high_state != LIFO_TEST_END + 1) ||
		      (coop_low_state != LIFO_TEST_END + 2) ||
		      (task_high_state != LIFO_TEST_END + 3) ||
		      (task_low_state != LIFO_TEST_END + 4),
		      "**** Unexpected timeout order");

}

ZTEST(pending, test_pending_timer)
{
	/*
	 * Main thread(test_main) priority was 9 but ztest thread runs at
	 * priority -1. To run the test smoothly make both main and ztest
	 * threads run at same priority level.
	 */
	k_thread_priority_set(k_current_get(), 9);

	struct offload_work offload2 = {0};

	k_work_init(&offload2.work_item, sync_threads);
	offload2.sem = &end_test_sem;
	k_work_submit_to_queue(&offload_work_q, &offload2.work_item);

	timer_end_tick = 0U;
	k_sem_give(&start_test_sem);    /* start timer tests */

	/*
	 * NOTE: The timer test is running in the context of high_task().
	 * Scheduling is expected to yield to high_task().  If high_task()
	 * does not pend as expected, then timer_end_tick will be non-zero.
	 */

	TC_PRINT("Testing preemptible thread waiting on timer ...\n");
	zassert_equal(timer_end_tick, 0, "Task did not pend on timer");

	/* Let the timer expire */
	k_sleep(K_SECONDS(2));

	zassert_false((timer_end_tick < timer_start_tick + NUM_SECONDS(1)),
			"Task waiting on timer error");

	zassert_equal(timer_data, NON_NULL_PTR,
				"Incorrect data from timer");

	k_sem_give(&end_test_sem);
}

/**
 * @}
 */

K_THREAD_DEFINE(TASK_LOW, PREEM_STACKSIZE, task_low, NULL, NULL, NULL,
		7, 0, 0);

K_THREAD_DEFINE(TASK_HIGH, PREEM_STACKSIZE, task_high, NULL, NULL, NULL,
		5, 0, 0);

ZTEST_SUITE(pending, NULL, NULL,
		ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
