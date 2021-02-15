/*
 * Copyright (c) 2016, 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Workqueue Tests
 * @defgroup kernel_workqueue_tests Workqueue
 * @ingroup all_tests
 * @{
 * @}
 */

#include <ztest.h>
#include <irq_offload.h>

#define TIMEOUT_MS 100
#define TIMEOUT K_MSEC(TIMEOUT_MS)
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)
#define NUM_OF_WORK 2
#define SYNC_SEM_INIT_VAL (0U)
#define COM_SEM_MAX_VAL (1U)
#define COM_SEM_INIT_VAL (0U)
#define MAX_WORK_Q_NUMBER 10
#define MY_PRIORITY 5

struct k_delayed_work work_item_delayed;
struct k_sem common_sema, sema_fifo_one, sema_fifo_two;
struct k_work work_item, work_item_1, work_item_2;
struct k_work_q work_q_max_number[MAX_WORK_Q_NUMBER];
K_THREAD_STACK_DEFINE(my_stack_area, STACK_SIZE);
K_THREAD_STACK_DEFINE(new_stack_area[MAX_WORK_Q_NUMBER], STACK_SIZE);

static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static K_THREAD_STACK_DEFINE(user_tstack, STACK_SIZE);
static struct k_work_q workq;
struct k_work_q work_q_1;
static struct k_work_q user_workq;
static ZTEST_BMEM struct k_work work[NUM_OF_WORK];
static struct k_delayed_work new_work;
static struct k_delayed_work delayed_work[NUM_OF_WORK], delayed_work_sleepy;
static struct k_work_poll triggered_work[NUM_OF_WORK];
static struct k_poll_event triggered_work_event[NUM_OF_WORK];
static struct k_poll_signal triggered_work_signal[NUM_OF_WORK];
static struct k_work_poll triggered_work_sleepy;
static struct k_poll_event triggered_work_sleepy_event;
static struct k_poll_signal triggered_work_sleepy_signal;
static struct k_sem sync_sema;
static struct k_sem dummy_sema;
static struct k_thread *main_thread;

/**
 * @brief Common function using like a handler for workqueue tests
 * API call in it means successful execution of that function
 *
 * @param unused of type k_work to make handler function accepted
 * by k_work_init
 *
 * @return N/A
 */
void common_work_handler(struct k_work *unused)
{
	k_sem_give(&sync_sema);
}

/**
 * @brief Test work item can take call back function defined by user
 * @details
 * Test Objective:
 * - Test a work item shall be supplied as a user-defined callback
 * function, and the handler function of a work item shall be able
 * to utilize any kernel API available to threads.
 *
 * Test techniques:
 * - Dynamic analysis and testing
 * - Interface testing
 *
 * Prerequisite Conditions:
 * - N/A
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Creating a work item, then add handler function to that work item.
 * -# Then process that work item.
 * -# To check that handler function executed successfully, we use semaphore
 * sync_sema with initial count 0.
 * -# Handler function gives semaphore, then we wait for that semaphore
 * from the test function body.
 * -# Check if semaphore was obtained successfully.
 * -# Set a work item's flag in pending state and append the item into workqueue.
 * -# Check if the queue is empty.
 *
 * Expected Test Result:
 * - The work item can be submit to workqueue whenever it is in right state.
 *
 * Pass/Fail Criteria:
 * - Successful if check points in test procedure are all passed,
 * otherwise failure.
 *
 * Assumptions and Constraints:
 * - N/A
 *
 * @see k_work_init()
 *
 * @ingroup kernel_workqueue_tests
 */
void test_work_item_supplied_with_func(void)
{
	uint32_t sem_count = 0;

	k_sem_reset(&sync_sema);

	/**TESTPOINT: init work item with a user-defined function*/
	k_work_init(&work_item, common_work_handler);
	k_work_submit_to_queue(&workq, &work_item);

	k_sem_take(&sync_sema, K_FOREVER);
	sem_count = k_sem_count_get(&sync_sema);
	zassert_equal(sem_count, COM_SEM_INIT_VAL, NULL);

	/* TESTPOINT: When a work item be added to a workqueue,
	 * it's flag will be in pending state, before the work item be processed,
	 * it cannot be append to a workqueue another time.
	 */
	zassert_false(k_work_pending(&work_item), NULL);
	k_work_submit_to_queue(&workq, &work_item);
	zassert_true(k_work_pending(&work_item), NULL);
	k_work_submit_to_queue(&workq, &work_item);

	/* Test the work item's callback function can only be invoked once */
	k_sem_take(&sync_sema, K_FOREVER);
	zassert_true(k_queue_is_empty(&workq.queue), NULL);
}


/**
 * @brief Test k_work_submit_to_user_queue API
 *
 * @details Funcion k_work_submit_to_user_queue() will return
 * -EBUSY: if the work item was already in some workqueue and
 * -ENOMEM: if no memory for thread resource pool allocation.
 * Create two situation to meet the error return value.
 *
 * @see k_work_submit_to_user_queue()
 * @ingroup kernel_workqueue_tests
 */
void test_k_work_submit_to_user_queue_fail(void)
{
	int ret = 0;

	k_sem_reset(&sync_sema);
	k_work_init(&work[0], common_work_handler);
	k_work_init(&work[1], common_work_handler);

	/* TESTPOINT: When a work item be added to a workqueue,
	 * it's flag will be in pending state, before the work item be processed,
	 * it cannot be append to a workqueue another time.
	 */
	k_work_submit_to_user_queue(&user_workq, &work[0]);
	k_work_submit_to_user_queue(&user_workq, &work[0]);

	/* Test the work item's callback function can only be invoked once */
	k_sem_take(&sync_sema, K_FOREVER);
	zassert_true(k_queue_is_empty(&user_workq.queue), NULL);

	/* use up the memory in resource pool */
	for (int i = 0; i < 100; i++) {
		ret = k_queue_alloc_append(&user_workq.queue, &work[1]);
		if (ret == -ENOMEM) {
			break;
		}
	}

	k_work_submit_to_user_queue(&user_workq, &work[0]);
	/* if memory is used up, the work cannot be append into the workqueue */
	zassert_false(k_work_pending(&work[0]), NULL);
}


/* Two handler functions fifo_work_first() and fifo_work_second
 * are made for two work items to test first in, first out.
 * It tests workqueue thread process work items
 * in first in, first out manner
 */
void fifo_work_first(struct k_work *unused)
{
	k_sem_take(&sema_fifo_one, K_FOREVER);
	k_sem_give(&sema_fifo_two);
}

void fifo_work_second(struct k_work *unused)
{
	k_sem_take(&sema_fifo_two, K_FOREVER);
}

/**
 * @brief Test kernel process work items in fifo way
 * @details To test it we use 2 functions-handlers.
 * - fifo_work_first() added to the work_item_1 and fifo_work_second()
 * added to the work_item_2.
 * - We expect that firstly should run work_item_1, and only then
 * will run work_item_2.
 * - To test it, we initialize semaphore sema_fifo_one
 * with count 1(available) and fifo_work_first() takes that semaphore.
 * fifo_work_second() is waiting for the semaphore sema_fifo_two,
 * which will be given from function fifo_work_first().
 * - If work_item_2() will try to be executed before work_item_1(),
 * will happen a timeout error.
 * - Because sema_fifo_two will be never obtained by fifo_work_second()
 * due to K_FOREVER macro in it while waiting for the semaphore.
 * @ingroup kernel_workqueue_tests
 */
void test_process_work_items_fifo(void)
{
	k_work_init(&work_item_1, fifo_work_first);
	k_work_init(&work_item_2, fifo_work_second);

	/**TESTPOINT: submit work items to the queue in fifo manner*/
	k_work_submit_to_queue(&workq, &work_item_1);
	k_work_submit_to_queue(&workq, &work_item_2);
}

/**
 * @brief Test submitting a delayed work item to a workqueue
 * @details
 * Test Objective:
 * - Test kernel support scheduling work item that is to be processed
 * after user defined period of time.
 *
 * Test techniques:
 * - Dynamic analysis and testing
 * - Interface testing
 *
 * Prerequisite Conditions:
 * - N/A
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# For that test is using semaphore sync_sema, with initial count 0.
 * -# In that test we measure actual spent time and compare it with time
 * which was measured by function k_delayed_work_remaining_get().
 * -# Using system clocks we measure actual spent time
 * in the period between delayed work submitted and delayed work
 * executed.
 * -# To know that delayed work was executed, we use semaphore.
 * -# Right after semaphore was given from handler function, we stop
 * measuring actual time.
 * -# Then compare results.
 *
 * Expected Test Result:
 * - The measured time results are in a very small range.
 *
 * Pass/Fail Criteria:
 * - Successful if check points in test procedure are all passed,
 * otherwise failure.
 *
 * Assumptions and Constraints:
 * - N/A
 *
 * @see k_delayed_work_init()
 *
 * @ingroup kernel_workqueue_tests
 */
void test_sched_delayed_work_item(void)
{
	int32_t ms_remain, ms_spent, start_time, stop_time, cycles_spent;
	int32_t ms_delta = 15;

	k_sem_reset(&sync_sema);

	/* TESTPOINT: init delayed work to be processed
	 * only after specific period of time
	 */
	k_delayed_work_init(&work_item_delayed, common_work_handler);
	/* align to tick before schedule */
	k_usleep(1);
	start_time = k_cycle_get_32();
	k_delayed_work_submit_to_queue(&workq, &work_item_delayed, TIMEOUT);
	ms_remain = k_delayed_work_remaining_get(&work_item_delayed);

	k_sem_take(&sync_sema, K_FOREVER);
	stop_time = k_cycle_get_32();
	cycles_spent = stop_time - start_time;
	ms_spent = (uint32_t)k_cyc_to_ms_floor32(cycles_spent);

	zassert_within(ms_spent, ms_remain, ms_delta, NULL);
}

/**
 * @brief Test define any number of workqueues
 * @details
 * Test Objective:
 * - Test application can define workqueues without any limit.
 *
 * Test techniques:
 * - Dynamic analysis and testing
 * - Interface testing
 *
 * Prerequisite Conditions:
 * - N/A
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Define any number of workqueus using a var of type
 * struct k_work_q.
 * -# Define and initialize maximum possible real number of the
 * workqueues available according to the stack size.
 * -# Test defines and initializes max number of the workqueues
 * and starts them.
 * -# Check if the number of workqueues defined is the same as the
 * number of times the definition operation was performed.
 *
 * Expected Test Result:
 * - The max number of workqueues be created.
 *
 * Pass/Fail Criteria:
 * - Successful if check points in test procedure are all passed,
 * otherwise failure.
 *
 * Assumptions and Constraints:
 * - N/A
 *
 * @see k_work_q_start()
 *
 * @ingroup kernel_workqueue_tests
 */
void test_workqueue_max_number(void)
{
	uint32_t work_q_num = 0;

	for (uint32_t i = 0; i < MAX_WORK_Q_NUMBER; i++) {
		work_q_num++;
		k_work_q_start(&work_q_max_number[i], new_stack_area[i],
			       K_THREAD_STACK_SIZEOF(new_stack_area[i]),
			       MY_PRIORITY);
	}
	zassert_true(work_q_num == MAX_WORK_Q_NUMBER,
		     "Max number of the defined work queues not reached, "
		     "real number of the created work queues is "
		     "%d, expected %d", work_q_num, MAX_WORK_Q_NUMBER);
}

static void work_sleepy(struct k_work *w)
{
	k_sleep(TIMEOUT);
	k_sem_give(&sync_sema);
}

static void work_handler(struct k_work *w)
{
	/* Just to show an API call on this will succeed */
	k_sem_init(&dummy_sema, 0, 1);

	k_sem_give(&sync_sema);
}

static void new_work_handler(struct k_work *w)
{
	k_sem_give(&sync_sema);
}

static void twork_submit_1(struct k_work_q *work_q, struct k_work *w,
			   k_work_handler_t handler)
{
	/**TESTPOINT: init via k_work_init*/
	k_work_init(w, handler);
	/**TESTPOINT: check pending after work init*/
	zassert_false(k_work_pending(w), NULL);

	if (work_q) {
		/**TESTPOINT: work submit to queue*/
		zassert_false(k_work_submit_to_user_queue(work_q, w),
			      "failed to submit to queue");
	} else {
		/**TESTPOINT: work submit to system queue*/
		k_work_submit(w);
	}
}

static void twork_submit(const void *data)
{
	struct k_work_q *work_q = (struct k_work_q *)data;

	for (int i = 0; i < NUM_OF_WORK; i++) {
		twork_submit_1(work_q, &work[i], work_handler);
	}
}

static void twork_submit_multipleq(void *data)
{
	struct k_work_q *work_q = (struct k_work_q *)data;

	if (IS_ENABLED(CONFIG_SOC_QEMU_ARC_HS)) {
		/* On this platform the wait for first submission to
		 * timeout returns after the pending work has been
		 * taken off the work queue.
		 */
		ztest_test_skip();
		return;
	}

	/**TESTPOINT: init via k_work_init*/
	k_delayed_work_init(&new_work, new_work_handler);
	zassert_equal(k_delayed_work_cancel(&new_work),
		      -EINVAL, NULL);

	/* align to tick before schedule */
	k_usleep(1);
	k_delayed_work_submit_to_queue(work_q, &new_work, TIMEOUT);
	zassert_true(k_delayed_work_pending(&new_work), NULL);

	zassert_equal(k_delayed_work_submit(&new_work, TIMEOUT),
		      -EADDRINUSE, NULL);

	/* wait for first submission to complete timeout, then sleep
	 * once to ensure the work thread gets to run (depending on
	 * timing it may or may not be submitted right after the
	 * timeout sleep returns.
	 */
	k_sleep(TIMEOUT);
	k_usleep(1);
	zassert_false(k_delayed_work_pending(&new_work), NULL);

	/* confirm legacy behavior that completed work item can't be
	 * submitted on a new queue.
	 */
	zassert_equal(k_delayed_work_submit(&new_work, TIMEOUT),
		      -EADDRINUSE, NULL);

	/* still can't if the (completed) work item is unsuccessfully
	 * cancelled
	 */
	zassert_equal(k_delayed_work_cancel(&new_work),
		      -EALREADY, NULL);
	zassert_equal(k_delayed_work_submit(&new_work, TIMEOUT),
		      -EADDRINUSE, NULL);

	/* but can if the work item is successfully cancelled */
	zassert_equal(k_delayed_work_submit_to_queue(work_q, &new_work,
						     TIMEOUT),
		      0, NULL);
	zassert_equal(k_delayed_work_cancel(&new_work),
		      0, NULL);
	zassert_equal(k_delayed_work_cancel(&new_work),
		      -EINVAL, NULL);
	zassert_equal(k_delayed_work_submit(&new_work, TIMEOUT),
		      0, NULL);
}

static void twork_resubmit(void *data)
{
	struct k_work_q *work_q = (struct k_work_q *)data;

	/**TESTPOINT: init via k_work_init*/
	k_delayed_work_init(&new_work, new_work_handler);

	k_delayed_work_submit_to_queue(work_q, &new_work, K_NO_WAIT);

	/* This is done to test a neagtive case when k_delayed_work_cancel()
	 * fails in k_delayed_work_submit_to_queue API. Removing work from it
	 * queue make sure that k_delayed_work_cancel() fails when the Work is
	 * resubmitted.
	 */
	k_queue_remove(&(new_work.work_q->queue), &(new_work.work));

	zassert_equal(k_delayed_work_submit_to_queue(work_q, &new_work,
				K_NO_WAIT),
				-EINVAL, NULL);

	k_sem_give(&sync_sema);
}

static void twork_resubmit_nowait(void *data)
{
	struct k_work_q *work_q = (struct k_work_q *)data;

	k_delayed_work_init(&delayed_work_sleepy, work_sleepy);
	k_delayed_work_init(&delayed_work[0], work_handler);
	k_delayed_work_init(&delayed_work[1], work_handler);

	zassert_equal(k_delayed_work_submit_to_queue(work_q,
						     &delayed_work_sleepy,
						     K_NO_WAIT),
		      0, NULL);
	zassert_equal(k_delayed_work_submit_to_queue(work_q, &delayed_work[0],
						     K_NO_WAIT),
		      0, NULL);
	zassert_equal(k_delayed_work_submit_to_queue(work_q, &delayed_work[1],
						     K_NO_WAIT),
		      0, NULL);

	/* No-wait submissions are pended immediately. */
	zassert_true(k_work_pending(&delayed_work_sleepy.work), NULL);
	zassert_true(k_work_pending(&delayed_work[0].work), NULL);
	zassert_true(k_work_pending(&delayed_work[1].work), NULL);

	/* Release sleeper, check other two still pending */
	do {
		k_usleep(1);
	} while (k_work_pending(&delayed_work_sleepy.work));
	zassert_false(k_work_pending(&delayed_work_sleepy.work), NULL);
	zassert_true(k_work_pending(&delayed_work[0].work), NULL);
	zassert_true(k_work_pending(&delayed_work[1].work), NULL);

	/* Verify order of pending */
	zassert_equal(k_queue_peek_head(&workq.queue),
		      &delayed_work[0].work, NULL);
	zassert_equal(k_queue_peek_tail(&workq.queue),
		      &delayed_work[1].work, NULL);

	/* Verify that resubmitting moves to end. */
	zassert_equal(k_delayed_work_submit_to_queue(work_q, &delayed_work[0],
						     K_NO_WAIT),
		      0, NULL);
	zassert_equal(k_queue_peek_head(&workq.queue),
		      &delayed_work[1].work, NULL);
	zassert_equal(k_queue_peek_tail(&workq.queue),
		      &delayed_work[0].work, NULL);

	k_sem_give(&sync_sema);
}

static void tdelayed_work_submit_1(struct k_work_q *work_q,
				   struct k_delayed_work *w,
				   k_work_handler_t handler)
{
	int32_t time_remaining;
	int32_t timeout_ticks;
	uint64_t tick_to_ms;

	/**TESTPOINT: init via k_delayed_work_init*/
	k_delayed_work_init(w, handler);
	/**TESTPOINT: check pending after delayed work init*/
	zassert_false(k_work_pending(&w->work), NULL);
	zassert_false(k_delayed_work_pending(w), NULL);
	/**TESTPOINT: check remaining timeout before submit*/
	zassert_equal(k_delayed_work_remaining_get(w), 0, NULL);

	/* align to tick before schedule */
	if (!k_is_in_isr()) {
		k_usleep(1);
	}
	if (work_q) {
		/**TESTPOINT: delayed work submit to queue*/
		zassert_true(k_delayed_work_submit_to_queue(work_q, w, TIMEOUT)
			     == 0, NULL);
	} else {
		/**TESTPOINT: delayed work submit to system queue*/
		zassert_true(k_delayed_work_submit(w, TIMEOUT) == 0, NULL);
	}

	zassert_false(k_work_pending(&w->work), NULL);
	zassert_true(k_delayed_work_pending(w), NULL);

	time_remaining = k_delayed_work_remaining_get(w);
	timeout_ticks = z_ms_to_ticks(TIMEOUT_MS);
	tick_to_ms = k_ticks_to_ms_floor64(timeout_ticks +  _TICK_ALIGN);

	/**TESTPOINT: check remaining timeout after submit */
	zassert_true(time_remaining <= tick_to_ms, NULL);

	timeout_ticks -= z_ms_to_ticks(15);
	tick_to_ms = k_ticks_to_ms_floor64(timeout_ticks);

	zassert_true(time_remaining >= tick_to_ms, NULL);

	/**TESTPOINT: check pending after delayed work submit*/
	zassert_false(k_work_pending(&w->work), NULL);
}

static void tdelayed_work_submit(const void *data)
{
	struct k_work_q *work_q = (struct k_work_q *)data;

	for (int i = 0; i < NUM_OF_WORK; i++) {
		tdelayed_work_submit_1(work_q, &delayed_work[i], work_handler);
	}
}

static void tdelayed_work_cancel(const void *data)
{
	struct k_work_q *work_q = (struct k_work_q *)data;
	int ret;

	k_delayed_work_init(&delayed_work_sleepy, work_sleepy);
	k_delayed_work_init(&delayed_work[0], work_handler);
	k_delayed_work_init(&delayed_work[1], work_handler);

	/* align to tick before schedule */
	if (!k_is_in_isr()) {
		k_usleep(1);
	}
	if (work_q) {
		ret = k_delayed_work_submit_to_queue(work_q,
						     &delayed_work_sleepy,
						     TIMEOUT);
		ret |= k_delayed_work_submit_to_queue(work_q, &delayed_work[0],
						      TIMEOUT);
		ret |= k_delayed_work_submit_to_queue(work_q, &delayed_work[1],
						      TIMEOUT);
	} else {
		ret = k_delayed_work_submit(&delayed_work_sleepy, TIMEOUT);
		ret |= k_delayed_work_submit(&delayed_work[0], TIMEOUT);
		ret |= k_delayed_work_submit(&delayed_work[1], TIMEOUT);
	}
	/*
	 * t0: delayed submit three work items, all with delay=TIMEOUT
	 * >t0: cancel delayed_work[0], expected cancellation success
	 * >t0+TIMEOUT: handling delayed_work_sleepy, which do k_sleep TIMEOUT
	 *              pending delayed_work[1], check pending flag, expected 1
	 *              cancel delayed_work[1], expected 0
	 * >t0+2*TIMEOUT: delayed_work_sleepy completed
	 *                delayed_work[1] completed
	 *                cancel delayed_work_sleepy, expected 0
	 */
	zassert_true(ret == 0, NULL);
	/**TESTPOINT: delayed work cancel when countdown*/
	zassert_false(k_work_pending(&delayed_work[0].work), NULL);
	zassert_true(k_delayed_work_pending(&delayed_work[0]), NULL);
	ret = k_delayed_work_cancel(&delayed_work[0]);
	zassert_true(ret == 0, NULL);
	/**TESTPOINT: check pending after delayed work cancel*/
	zassert_false(k_work_pending(&delayed_work[0].work), NULL);
	zassert_false(k_delayed_work_pending(&delayed_work[0]), NULL);
	if (!k_is_in_isr()) {
		/*wait for handling work_sleepy*/
		k_sleep(TIMEOUT);
		/**TESTPOINT: check pending when work pending*/
		zassert_true(k_work_pending(&delayed_work[1].work), NULL);
		zassert_true(k_delayed_work_pending(&delayed_work[1]), NULL);
		/**TESTPOINT: delayed work cancel when pending*/
		ret = k_delayed_work_cancel(&delayed_work[1]);
		zassert_equal(ret, 0, NULL);
		zassert_false(k_work_pending(&delayed_work[1].work), NULL);
		zassert_false(k_delayed_work_pending(&delayed_work[1]), NULL);
		k_sem_give(&sync_sema);
		/*wait for completed work_sleepy and delayed_work[1]*/
		k_sleep(TIMEOUT);
		/**TESTPOINT: check pending when work completed*/
		zassert_false(k_work_pending(&delayed_work_sleepy.work),
					NULL);
		/**TESTPOINT: delayed work cancel when completed */
		ret = k_delayed_work_cancel(&delayed_work_sleepy);
		zassert_equal(ret, -EALREADY, NULL);
	}
	/*work items not cancelled: delayed_work[1], delayed_work_sleepy*/
}

static void ttriggered_work_submit(const void *data)
{
	struct k_work_q *work_q = (struct k_work_q *)data;

	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_poll_signal_init(&triggered_work_signal[i]);
		k_poll_event_init(&triggered_work_event[i],
				  K_POLL_TYPE_SIGNAL,
				  K_POLL_MODE_NOTIFY_ONLY,
				  &triggered_work_signal[i]);

		/**TESTPOINT: init via k_work_poll_init*/
		k_work_poll_init(&triggered_work[i], work_handler);
		/**TESTPOINT: check pending after triggered work init*/
		zassert_false(k_work_pending(&triggered_work[i].work),
					NULL);
		if (work_q) {
			/**TESTPOINT: triggered work submit to queue*/
			zassert_true(k_work_poll_submit_to_queue(
						work_q,
						&triggered_work[i],
						&triggered_work_event[i], 1,
						K_FOREVER) == 0, NULL);
		} else {
			/**TESTPOINT: triggered work submit to system queue*/
			zassert_true(k_work_poll_submit(
						&triggered_work[i],
						&triggered_work_event[i], 1,
						K_FOREVER) == 0, NULL);
		}

		/**TESTPOINT: check pending after triggered work submit*/
		zassert_false(k_work_pending(&triggered_work[i].work),
				NULL);
	}

	for (int i = 0; i < NUM_OF_WORK; i++) {
		/**TESTPOINT: trigger work execution*/
		zassert_true(k_poll_signal_raise(
				&triggered_work_signal[i], 1) == 0,
				NULL);
		/**TESTPOINT: check pending after sending signal */
		zassert_true(k_work_pending(&triggered_work[i].work) != 0,
				NULL);
	}
}

static void ttriggered_work_cancel(const void *data)
{
	struct k_work_q *work_q = (struct k_work_q *)data;
	int ret;

	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_poll_signal_init(&triggered_work_signal[i]);
		k_poll_event_init(&triggered_work_event[i],
				  K_POLL_TYPE_SIGNAL,
				  K_POLL_MODE_NOTIFY_ONLY,
				  &triggered_work_signal[i]);

		k_work_poll_init(&triggered_work[i], work_handler);
	}

	k_poll_signal_init(&triggered_work_sleepy_signal);
	k_poll_event_init(&triggered_work_sleepy_event,
			  K_POLL_TYPE_SIGNAL,
			  K_POLL_MODE_NOTIFY_ONLY,
			  &triggered_work_sleepy_signal);

	k_work_poll_init(&triggered_work_sleepy, work_sleepy);

	if (work_q) {
		ret = k_work_poll_submit_to_queue(work_q,
						  &triggered_work_sleepy,
						  &triggered_work_sleepy_event,
						  1, K_FOREVER);
		ret |= k_work_poll_submit_to_queue(work_q,
						   &triggered_work[0],
						   &triggered_work_event[0], 1,
						   K_FOREVER);
		ret |= k_work_poll_submit_to_queue(work_q,
						   &triggered_work[1],
						   &triggered_work_event[1], 1,
						   K_FOREVER);
	} else {
		ret = k_work_poll_submit(&triggered_work_sleepy,
					 &triggered_work_sleepy_event, 1,
					 K_FOREVER);
		ret |= k_work_poll_submit(&triggered_work[0],
					  &triggered_work_event[0], 1,
					  K_FOREVER);
		ret |= k_work_poll_submit(&triggered_work[1],
					  &triggered_work_event[1], 1,
					  K_FOREVER);
	}
	/* Check if all submission succeeded */
	zassert_true(ret == 0, NULL);

	/**TESTPOINT: triggered work cancel when waiting for event*/
	ret = k_work_poll_cancel(&triggered_work[0]);
	zassert_true(ret == 0, NULL);

	/**TESTPOINT: check pending after triggerd work cancel*/
	ret = k_work_pending(&triggered_work[0].work);
	zassert_true(ret == 0, NULL);

	/* Trigger work #1 */
	ret = k_poll_signal_raise(&triggered_work_signal[1], 1);
	zassert_true(ret == 0, NULL);

	/**TESTPOINT: check pending after sending signal */
	ret = k_work_pending(&triggered_work[1].work);
	zassert_true(ret != 0, NULL);

	/**TESTPOINT: triggered work cancel when pending for event*/
	ret = k_work_poll_cancel(&triggered_work[1]);
	zassert_true(ret == -EINVAL, NULL);

	/* Trigger sleepy work */
	ret = k_poll_signal_raise(&triggered_work_sleepy_signal, 1);
	zassert_true(ret == 0, NULL);

	if (!k_is_in_isr()) {
		/*wait for completed work_sleepy and triggered_work[1]*/
		k_msleep(2 * TIMEOUT_MS);

		/**TESTPOINT: check pending when work completed*/
		ret = k_work_pending(&triggered_work_sleepy.work);
		zassert_true(ret == 0, NULL);

		/**TESTPOINT: delayed work cancel when completed*/
		ret = k_work_poll_cancel(&triggered_work_sleepy);
		zassert_true(ret == -EINVAL, NULL);

	}

	/*work items not cancelled: triggered_work[1], triggered_work_sleepy*/
}

/**
 * @brief Test work queue start before submit
 * @details
 * Test Objective:
 * - Test the workqueue is initialized by defining the stack area used by
 * its thread and then calling k_work_q_start().
 *
 * Test techniques:
 * - Dynamic analysis and testing
 * - Interface testing
 *
 * Prerequisite Conditions:
 * - N/A
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Call the k_work_q_start() to create a work queue which has it's own
 * thread that processes the work items in the queue.
 *
 * Expected Test Result:
 * - The user-defined work queue was created successfully.
 *
 * Pass/Fail Criteria:
 * - Successful if check points in test procedure are all passed,
 *otherwise failure.
 *
 * Assumptions and Constraints:
 * - N/A
 *
 * @see k_work_q_start()
 *
 * @ingroup kernel_workqueue_tests
 */
void test_workq_start_before_submit(void)
{
	k_work_q_start(&workq, tstack, STACK_SIZE,
		       CONFIG_MAIN_THREAD_PRIORITY);
	zassert_equal(strcmp(k_thread_name_get(&workq.thread), "workqueue"), 0,
		"workq thread creat failed");
}

/**
 * @brief Test user mode work queue start before submit
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_q_user_start()
 */
void test_user_workq_start_before_submit(void)
{
	k_work_q_user_start(&user_workq, user_tstack, STACK_SIZE,
			    CONFIG_MAIN_THREAD_PRIORITY);
}

/**
 * @brief Setup object permissions before test_user_workq_granted_access()
 *
 * @ingroup kernel_workqueue_tests
 */
void test_user_workq_granted_access_setup(void)
{
	/* Subsequent test cases will have access to the dummy_sema,
	 * but not the user workqueue since it already started.
	 */
	k_object_access_grant(&dummy_sema, main_thread);
}

/**
 * @brief Test user mode grant workqueue permissions
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_q_object_access_grant()
 */
void test_user_workq_granted_access(void)
{
	k_object_access_grant(&dummy_sema, &user_workq.thread);
}

/**
 * @brief Test work submission to work queue
 * @ingroup kernel_workqueue_tests
 * @see k_work_init(), k_work_pending(), k_work_submit_to_queue(),
 * k_work_submit()
 */
void test_work_submit_to_queue_thread(void)
{
	k_sem_reset(&sync_sema);
	twork_submit(&workq);
	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}
}

/**
 * @brief Test work submission to work queue (user mode)
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_init(), k_work_pending(), k_work_submit_to_queue(),
 * k_work_submit()
 */
void test_user_work_submit_to_queue_thread(void)
{
	k_sem_reset(&sync_sema);
	twork_submit(&user_workq);
	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}
}

/**
 * @brief Test submission of work to multiple queues
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_delayed_work_init(), k_delayed_work_submit_to_queue(),
 * k_delayed_work_submit()
 */
void test_work_submit_to_multipleq(void)
{
	k_sem_reset(&sync_sema);
	twork_submit_multipleq(&workq);
	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}
}

/**
 * @brief Test work queue resubmission
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_queue_remove(), k_delayed_work_init(),
 * k_delayed_work_submit_to_queue()
 */
void test_work_resubmit_to_queue(void)
{
	k_sem_reset(&sync_sema);
	twork_resubmit(&workq);
	k_sem_take(&sync_sema, K_FOREVER);
}

/**
 * @brief Test work queue resubmission behavior without wait
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_queue_remove(), k_delayed_work_init(),
 * k_delayed_work_submit_to_queue()
 */
void test_work_resubmit_nowait_to_queue(void)
{
	k_sem_reset(&sync_sema);
	twork_resubmit_nowait(&workq);
	k_sem_take(&sync_sema, K_FOREVER);
}

/**
 * @brief Test work submission to queue from ISR context
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_init(), k_work_pending(), k_work_submit_to_queue(),
 * k_work_submit()
 */
void test_work_submit_to_queue_isr(void)
{
	k_sem_reset(&sync_sema);
	irq_offload(twork_submit, (const void *)&workq);
	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}
}

/**
 * @brief Test work submission to queue
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_init(), k_work_pending(), k_work_submit_to_queue(),
 * k_work_submit()
 */
void test_work_submit_thread(void)
{
	k_sem_reset(&sync_sema);
	twork_submit(NULL);
	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}
}

/**
 * @brief Test work submission from ISR context
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_init(), k_work_pending(), k_work_submit_to_queue(),
 * k_work_submit()
 */
void test_work_submit_isr(void)
{
	k_sem_reset(&sync_sema);
	irq_offload(twork_submit, NULL);
	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}
}

static void work_handler_resubmit(struct k_work *w)
{
	k_sem_give(&sync_sema);

	if (k_sem_count_get(&sync_sema) < NUM_OF_WORK) {
		k_work_submit(w);
	}
}

/**
 * @brief Test resubmitting work item in callback
 * @details
 * Test Objective:
 * - Test work submission to queue from handler context, resubmitting
 * a work item during execution of its callback.
 *
 * Test techniques:
 * - Dynamic analysis and testing
 * - Functional and black box testing
 * - Interface testing
 *
 * Prerequisite Conditions:
 * - N/A
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Uses sync_sema semaphore with initial count 0.
 * -# Verifies that it is possible during execution of the handler
 * function, resubmit a work item from that handler function.
 * -# twork_submit_1() initializes a work item with handler function.
 * -# Then handler function gives a semaphore sync_sema.
 * -# Then in test main body using for() loop, we are waiting for that
 * semaphore.
 * -# When semaphore given, handler function checks count of the semaphore
 * and submits work one more time.
 *
 * Expected Test Result:
 * - The work item is recommitted. The callback will normally terminated
 * according to the exit condition(semaphore count).
 *
 * Pass/Fail Criteria:
 * - Successful if check points in test procedure are all passed,
 * otherwise failure.
 *
 * Assumptions and Constraints:
 * - N/A
 *
 * @see k_work_init(), k_work_pending(), k_work_submit_to_queue(),
 * k_work_submit()
 *
 * @ingroup kernel_workqueue_tests
 */
void test_work_submit_handler(void)
{
	k_sem_reset(&sync_sema);
	twork_submit_1(NULL, &work[0], work_handler_resubmit);
	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}
}

/**
 * @brief Test delayed work submission to queue
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_delayed_work_init(), k_work_pending(),
 * k_delayed_work_remaining_get(), k_delayed_work_submit_to_queue(),
 * k_delayed_work_submit()
 */
void test_delayed_work_submit_to_queue_thread(void)
{
	k_sem_reset(&sync_sema);
	tdelayed_work_submit(&workq);
	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}
}

/**
 * @brief Test delayed work submission to queue in ISR context
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_delayed_work_init(), k_work_pending(),
 * k_delayed_work_remaining_get(), k_delayed_work_submit_to_queue(),
 * k_delayed_work_submit()
 */
void test_delayed_work_submit_to_queue_isr(void)
{
	k_sem_reset(&sync_sema);
	irq_offload(tdelayed_work_submit, (const void *)&workq);
	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}
}

/**
 * @brief Test delayed work submission
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_delayed_work_init(), k_work_pending(),
 * k_delayed_work_remaining_get(), k_delayed_work_submit_to_queue(),
 * k_delayed_work_submit()
 */
void test_delayed_work_submit_thread(void)
{
	k_sem_reset(&sync_sema);
	tdelayed_work_submit(NULL);
	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}
}

/**
 * @brief Test delayed work submission from ISR context
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_delayed_work_init(), k_work_pending(),
 * k_delayed_work_remaining_get(), k_delayed_work_submit_to_queue(),
 * k_delayed_work_submit()
 */
void test_delayed_work_submit_isr(void)
{
	k_sem_reset(&sync_sema);
	irq_offload(tdelayed_work_submit, NULL);
	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}
}

static void delayed_work_handler_resubmit(struct k_work *w)
{
	struct k_delayed_work *delayed_w = (struct k_delayed_work *)w;

	k_sem_give(&sync_sema);

	if (k_sem_count_get(&sync_sema) < NUM_OF_WORK) {
		/**TESTPOINT: check if work can be resubmit from handler */
		zassert_false(k_delayed_work_submit(delayed_w, TIMEOUT), NULL);
	}
}

/**
 * @brief Test delayed work submission to queue from handler context
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_delayed_work_init(), k_work_pending(),
 * k_delayed_work_remaining_get(), k_delayed_work_submit_to_queue(),
 * k_delayed_work_submit()
 */
void test_delayed_work_submit_handler(void)
{
	k_sem_reset(&sync_sema);
	tdelayed_work_submit_1(NULL, &delayed_work[0],
			       delayed_work_handler_resubmit);
	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}
	k_delayed_work_cancel(&delayed_work[0]);
}

/**
 * @brief Test delayed work cancel from work queue
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_delayed_work_cancel(), k_work_pending()
 */
void test_delayed_work_cancel_from_queue_thread(void)
{
	k_sem_reset(&sync_sema);
	tdelayed_work_cancel(&workq);
	/*wait for work items that could not be cancelled*/
	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}
}

/**
 * @brief Test delayed work cancel from work queue from ISR context
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_delayed_work_cancel(), k_work_pending()
 */
void test_delayed_work_cancel_from_queue_isr(void)
{
	k_sem_reset(&sync_sema);
	irq_offload(tdelayed_work_cancel, (const void *)&workq);
	/*wait for work items that could not be cancelled*/
	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}
}

/**
 * @brief Test delayed work cancel
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_delayed_work_cancel(), k_work_pending()
 */
void test_delayed_work_cancel_thread(void)
{
	k_sem_reset(&sync_sema);
	tdelayed_work_cancel(NULL);
	/*wait for work items that could not be cancelled*/
	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}
}

/**
 * @brief Test delayed work cancel from ISR context
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_delayed_work_cancel(), k_work_pending()
 */
void test_delayed_work_cancel_isr(void)
{
	k_sem_reset(&sync_sema);
	irq_offload(tdelayed_work_cancel, NULL);
	/*wait for work items that could not be cancelled*/
	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}
}

/**
 * @brief Test triggered work submission to queue
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_poll_init(), k_work_pending(),
 * k_work_poll_submit_to_queue(),
 * k_work_poll_submit()
 */
void test_triggered_work_submit_to_queue_thread(void)
{
	k_sem_reset(&sync_sema);
	ttriggered_work_submit(&workq);
	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}
}

/**
 * @brief Test triggered work submission to queue in ISR context
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_poll_init(), k_work_pending(),
 * k_work_poll_submit_to_queue(),
 * k_work_poll_submit()
 */
void test_triggered_work_submit_to_queue_isr(void)
{
	k_sem_reset(&sync_sema);
	irq_offload(ttriggered_work_submit, (const void *)&workq);
	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}
}

/**
 * @brief Test triggered work submission
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_poll_init(), k_work_pending(),
 * k_work_poll_submit_to_queue(),
 * k_work_poll_submit()
 */
void test_triggered_work_submit_thread(void)
{
	k_sem_reset(&sync_sema);
	ttriggered_work_submit(NULL);
	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}
}

/**
 * @brief Test triggered work submission from ISR context
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_poll_init(), k_work_pending(),
 * k_work_poll_submit_to_queue(),
 * k_work_poll_submit()
 */
void test_triggered_work_submit_isr(void)
{
	k_sem_reset(&sync_sema);
	irq_offload(ttriggered_work_submit, NULL);
	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}
}

/**
 * @brief Test triggered work cancel from work queue
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_poll_cancel(), k_work_pending()
 */
void test_triggered_work_cancel_from_queue_thread(void)
{
	k_sem_reset(&sync_sema);
	ttriggered_work_cancel(&workq);
	/*wait for work items that could not be cancelled*/
	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}
}

/**
 * @brief Test triggered work cancel from work queue from ISR context
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_poll_cancel(), k_work_pending()
 */
void test_triggered_work_cancel_from_queue_isr(void)
{
	k_sem_reset(&sync_sema);
	irq_offload(ttriggered_work_cancel, (const void *)&workq);
	/*wait for work items that could not be cancelled*/
	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}
}

/**
 * @brief Test triggered work cancel
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_poll_cancel(), k_work_pending()
 */
void test_triggered_work_cancel_thread(void)
{
	k_sem_reset(&sync_sema);
	ttriggered_work_cancel(NULL);
	/*wait for work items that could not be cancelled*/
	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}
}

/**
 * @brief Test triggered work cancel from ISR context
 *
 * @ingroup kernel_workqueue_tests
 *
 * @see k_work_poll_cancel(), k_work_pending()
 */
void test_triggered_work_cancel_isr(void)
{
	k_sem_reset(&sync_sema);
	irq_offload(ttriggered_work_cancel, NULL);
	/*wait for work items that could not be cancelled*/
	for (int i = 0; i < NUM_OF_WORK; i++) {
		k_sem_take(&sync_sema, K_FOREVER);
	}
}

void new_common_work_handler(struct k_work *unused)
{
	k_sem_give(&sync_sema);
	k_sem_take(&sema_fifo_two, K_FOREVER);
}

/**
 * @brief Test cancel already processed work item
 * @details That test is created to increase coverage and to check that we can
 * cancel already processed delayed work item.
 * @ingroup kernel_workqueue_tests
 * @see k_delayed_work_cancel()
 */
void test_cancel_processed_work_item(void)
{
	int ret;

	k_sem_reset(&sync_sema);
	k_sem_reset(&sema_fifo_two);

	k_delayed_work_init(&work_item_delayed, common_work_handler);

	ret = k_delayed_work_cancel(&work_item_delayed);
	zassert_true(ret == -EINVAL, NULL);
	k_delayed_work_submit_to_queue(&workq, &work_item_delayed, TIMEOUT);
	k_sem_take(&sync_sema, K_FOREVER);
	k_sem_give(&sema_fifo_two);

	/**TESTPOINT: try to delay already processed work item*/
	ret = k_delayed_work_cancel(&work_item_delayed);

	zassert_true(ret == -EALREADY, NULL);
	k_sleep(TIMEOUT);
}

void test_main(void)
{
	main_thread = k_current_get();
	k_thread_access_grant(main_thread, &sync_sema, &user_workq.thread,
			      &user_workq.queue,
			      &user_tstack);
	k_sem_init(&sync_sema, SYNC_SEM_INIT_VAL, NUM_OF_WORK);
	k_sem_init(&sema_fifo_one, COM_SEM_MAX_VAL, COM_SEM_MAX_VAL);
	k_sem_init(&sema_fifo_two, COM_SEM_INIT_VAL, COM_SEM_MAX_VAL);
	k_thread_system_pool_assign(k_current_get());

	ztest_test_suite(workqueue_api,
			 /* Do not disturb the ordering of these test cases */
			 ztest_unit_test(test_workq_start_before_submit),
			 ztest_user_unit_test(test_user_workq_start_before_submit),
			 ztest_unit_test(test_user_workq_granted_access_setup),
			 ztest_user_unit_test(test_user_workq_granted_access),
			/* End order-important tests */
			 ztest_1cpu_unit_test(test_work_submit_to_multipleq),
			 ztest_unit_test(test_work_resubmit_to_queue),
			 ztest_unit_test(test_work_resubmit_nowait_to_queue),
			 ztest_1cpu_unit_test(test_work_submit_to_queue_thread),
			 ztest_1cpu_unit_test(test_work_submit_to_queue_isr),
			 ztest_1cpu_unit_test(test_work_submit_thread),
			 ztest_1cpu_unit_test(test_work_submit_isr),
			 ztest_1cpu_unit_test(test_work_submit_handler),
			 ztest_1cpu_user_unit_test(test_user_work_submit_to_queue_thread),
			 ztest_1cpu_unit_test(test_delayed_work_submit_to_queue_thread),
			 ztest_1cpu_unit_test(test_delayed_work_submit_to_queue_isr),
			 ztest_1cpu_unit_test(test_delayed_work_submit_thread),
			 ztest_1cpu_unit_test(test_delayed_work_submit_isr),
			 ztest_1cpu_unit_test(test_delayed_work_submit_handler),
			 ztest_1cpu_unit_test(test_delayed_work_cancel_from_queue_thread),
			 ztest_1cpu_unit_test(test_delayed_work_cancel_from_queue_isr),
			 ztest_1cpu_unit_test(test_delayed_work_cancel_thread),
			 ztest_1cpu_unit_test(test_delayed_work_cancel_isr),
			 ztest_1cpu_unit_test(test_triggered_work_submit_to_queue_thread),
			 ztest_1cpu_unit_test(test_triggered_work_submit_to_queue_isr),
			 ztest_1cpu_unit_test(test_triggered_work_submit_thread),
			 ztest_1cpu_unit_test(test_triggered_work_submit_isr),
			 ztest_1cpu_unit_test(test_triggered_work_cancel_from_queue_thread),
			 ztest_1cpu_unit_test(test_triggered_work_cancel_from_queue_isr),
			 ztest_1cpu_unit_test(test_triggered_work_cancel_thread),
			 ztest_1cpu_unit_test(test_triggered_work_cancel_isr),
			 ztest_unit_test(test_work_item_supplied_with_func),
			 ztest_user_unit_test(test_k_work_submit_to_user_queue_fail),
			 ztest_unit_test(test_process_work_items_fifo),
			 ztest_unit_test(test_sched_delayed_work_item),
			 ztest_unit_test(test_workqueue_max_number),
			 ztest_unit_test(test_cancel_processed_work_item));
	ztest_run_test_suite(workqueue_api);
}
