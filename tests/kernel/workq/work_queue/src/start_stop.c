/*
 * Copyright (c) 2024 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define NUM_TEST_ITEMS 10
/* In fact, each work item could take up to this value */
#define WORK_ITEM_WAIT_ALIGNED                                                                     \
	k_ticks_to_ms_floor64(k_ms_to_ticks_ceil32(CONFIG_TEST_WORK_ITEM_WAIT_MS) + _TICK_ALIGN)
#define CHECK_WAIT ((NUM_TEST_ITEMS + 1) * WORK_ITEM_WAIT_ALIGNED)

static K_THREAD_STACK_DEFINE(work_q_stack, 1024 + CONFIG_TEST_EXTRA_STACK_SIZE);

static void work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	k_msleep(CONFIG_TEST_WORK_ITEM_WAIT_MS);
}

ZTEST(workqueue_api, test_k_work_queue_start_stop)
{
	size_t i;
	struct k_work work;
	struct k_work_q work_q = {0};
	struct k_work works[NUM_TEST_ITEMS];
	struct k_work_queue_config cfg = {
		.name = "test_work_q",
		.no_yield = true,
	};

	zassert_equal(k_work_queue_stop(&work_q, K_FOREVER), -EALREADY,
		      "Succeeded to stop work queue on non-initialized work queue");
	k_work_queue_start(&work_q, work_q_stack, K_THREAD_STACK_SIZEOF(work_q_stack),
			   K_PRIO_PREEMPT(4), &cfg);

	for (i = 0; i < NUM_TEST_ITEMS; i++) {
		k_work_init(&works[i], work_handler);
		zassert_equal(k_work_submit_to_queue(&work_q, &works[i]), 1,
			      "Failed to submit work item");
	}

	/* Wait for the work item to complete */
	k_sleep(K_MSEC(CHECK_WAIT));

	zassert_equal(k_work_queue_stop(&work_q, K_FOREVER), -EBUSY,
		      "Succeeded to stop work queue while it is running & not plugged");
	zassert_true(k_work_queue_drain(&work_q, true) >= 0, "Failed to drain & plug work queue");
	zassert_ok(k_work_queue_stop(&work_q, K_FOREVER), "Failed to stop work queue");

	k_work_init(&work, work_handler);
	zassert_equal(k_work_submit_to_queue(&work_q, &work), -ENODEV,
		      "Succeeded to submit work item to non-initialized work queue");
}

ZTEST(workqueue_api, test_k_work_queue_stop_sys_thread)
{
	struct k_work_q work_q = {0};
	struct k_work_queue_config cfg = {
		.name = "test_work_q",
		.no_yield = true,
		.essential = true,
	};

	k_work_queue_start(&work_q, work_q_stack, K_THREAD_STACK_SIZEOF(work_q_stack),
			   K_PRIO_PREEMPT(4), &cfg);

	zassert_true(k_work_queue_drain(&work_q, true) >= 0, "Failed to drain & plug work queue");
	zassert_equal(-ENOTSUP, k_work_queue_stop(&work_q, K_FOREVER), "Failed to stop work queue");
}


#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

static K_THREAD_STACK_DEFINE(run_stack, STACK_SIZE);

static void run_q_main(void *workq_ptr, void *sem_ptr, void *p3)
{
	ARG_UNUSED(p3);

	struct k_work_q *queue = (struct k_work_q *)workq_ptr;
	struct k_sem *sem = (struct k_sem *)sem_ptr;

	struct k_work_queue_config cfg = {
		.name = "wq.run_q",
		.no_yield = true,
	};

	k_work_queue_run(queue, &cfg);

	k_sem_give(sem);
}

ZTEST(workqueue_api, test_k_work_queue_run_stop)
{
	int rc;
	size_t i;
	struct k_thread thread;
	struct k_work work;
	struct k_work_q work_q = {0};
	struct k_work works[NUM_TEST_ITEMS];
	struct k_sem ret_sem;

	k_sem_init(&ret_sem, 0, 1);

	(void)k_thread_create(&thread, run_stack, STACK_SIZE, run_q_main, &work_q, &ret_sem, NULL,
			      K_PRIO_COOP(3), 0, K_FOREVER);

	k_thread_start(&thread);

	k_sleep(K_MSEC(CHECK_WAIT));

	for (i = 0; i < NUM_TEST_ITEMS; i++) {
		k_work_init(&works[i], work_handler);
		zassert_equal(k_work_submit_to_queue(&work_q, &works[i]), 1,
			      "Failed to submit work item");
	}

	/* Wait for the work item to complete */
	k_sleep(K_MSEC(CHECK_WAIT));

	zassert_equal(k_work_queue_stop(&work_q, K_FOREVER), -EBUSY,
		      "Succeeded to stop work queue while it is running & not plugged");
	zassert_true(k_work_queue_drain(&work_q, true) >= 0, "Failed to drain & plug work queue");
	zassert_ok(k_work_queue_stop(&work_q, K_FOREVER), "Failed to stop work queue");

	k_work_init(&work, work_handler);
	zassert_equal(k_work_submit_to_queue(&work_q, &work), -ENODEV,
		      "Succeeded to submit work item to non-initialized work queue");

	/* Take the semaphore the other thread released once done running the queue */
	rc = k_sem_take(&ret_sem, K_MSEC(1));
	zassert_equal(rc, 0);
}

ZTEST_SUITE(workqueue_api, NULL, NULL, NULL, NULL, NULL);
