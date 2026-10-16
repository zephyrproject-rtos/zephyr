/*
 * Copyright (c) 2025 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/workq.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(workq_tests, LOG_LEVEL_DBG);

#define WORKQ_TEST_STACK_SIZE 2048

WORKQ_DEFINE(test_define_workq);
WORKQ_THREAD_DEFINE(test_define_wqt, test_define_workq, WORKQ_TEST_STACK_SIZE, 0);

struct container {
	struct work_item item;
	struct k_sem *sem;
};

static void work_fn(struct work_item *item)
{
	struct container *c = CONTAINER_OF(item, struct container, item);

	k_sem_give(c->sem);
	k_free(c);
}

ZTEST(basic, test_macro_defined_workq)
{
	zassert_true(test_define_workq.flags == WORKQ_FLAG_OPEN, "workq should be open");
	zassert_true(test_define_wqt.flags & WORKQ_THREAD_FLAG_INITIALIZED,
			"workq thread should be initialized");
	zassert_true(test_define_wqt.flags & WORKQ_THREAD_FLAG_RUNNING,
			"workq thread should be running");
	zassert_true(workq_run(&test_define_workq, K_NO_WAIT) == -EAGAIN,
			"workq have no work, should return -EAGAIN");

	zassert_true(workq_thread_start(&test_define_wqt) == -EALREADY,
			"workq thread already started, should return -EALREADY");
	zassert_true(workq_thread_stop(&test_define_wqt, K_MSEC(10)) == 0,
			"workq thread should stop");
}

ZTEST(basic, test_setup)
{
	struct k_work_q oq;
	struct workq q;
	struct workq_thread wqt;

	LOG_INF("----------------------------");
	LOG_INF("sizeof k_work %zu", sizeof(struct k_work));
	LOG_INF("sizeof k_work_delayable %zu", sizeof(struct k_work_delayable));
	LOG_INF("sizeof k_workq %zu", sizeof(struct k_work_q));
	LOG_INF("----------------------------");
	LOG_INF("sizeof work %zu", sizeof(struct work_item));
	LOG_INF("sizeof q %zu", sizeof(struct workq));
	LOG_INF("sizeof wqt %zu", sizeof(struct workq_thread));
	LOG_INF("----------------------------");
	LOG_INF("sizeof workq.flags %zu", sizeof(q.flags));
	LOG_INF("sizeof workq.lock %zu", sizeof(q.lock));
	LOG_INF("sizeof workq.timeout %zu", sizeof(q.timeout));
	LOG_INF("sizeof workq.active %zu", sizeof(q.active));
	LOG_INF("sizeof workq.pending %zu", sizeof(q.pending));
	LOG_INF("sizeof workq.delayed %zu", sizeof(q.delayed));
	LOG_INF("sizeof workq.idle %zu", sizeof(q.idle));
	LOG_INF("sizeof workq.drain %zu", sizeof(q.drain));
	LOG_INF("----------------------------");
	LOG_INF("sizeof wqt.lock %zu", sizeof(wqt.lock));
	LOG_INF("sizeof wqt.wq %zu", sizeof(wqt.wq));
	LOG_INF("sizeof wqt.flags %zu", sizeof(wqt.flags));
	LOG_INF("sizeof wqt.cfg %zu", sizeof(wqt.cfg));
	LOG_INF("sizeof wqt.thread %zu", sizeof(wqt.thread));
	LOG_INF("sizeof wqt.stack %zu", sizeof(wqt.stack));
	LOG_INF("sizeof wqt.stack_size %zu", sizeof(wqt.stack_size));
	LOG_INF("----------------------------");
	LOG_INF("sizeof k_work_q.flags %zu", sizeof(oq.flags));
	LOG_INF("sizeof k_work_q.thread %zu", sizeof(oq.thread));
	LOG_INF("sizeof k_work_q.thread_id %zu", sizeof(oq.thread_id));
	LOG_INF("sizeof k_work_q.pending %zu", sizeof(oq.pending));
	LOG_INF("sizeof k_work_q.notifyq %zu", sizeof(oq.notifyq));
	LOG_INF("sizeof k_work_q.drainq %zu", sizeof(oq.drainq));
	LOG_INF("----------------------------");

	workq_init(&q);
	zassert_true(workq_run(&q, K_NO_WAIT) == -EAGAIN,
			"workq have no work, should return -EAGAIN");
}

ZTEST(basic, test_submit)
{
	struct workq q;
	struct k_sem sem;
	struct container *c;

	k_sem_init(&sem, 0, 3);
	workq_init(&q);

	for (size_t i = 0; i < 3; i++) {
		c = k_malloc(sizeof(struct container));
		zassert_not_null(c, "Failed to allocate memory for container");
		c->sem = &sem;
		work_init(&c->item, work_fn);
		zassert_true(workq_submit(&q, &c->item) == 0, "workq_submit failed");
	}
	for (size_t i = 0; i < 3; i++) {
		zassert_ok(workq_run(&q, K_NO_WAIT), "workq_run failed");
		zassert_ok(k_sem_take(&sem, K_NO_WAIT), "work_fn not called");
	}
}

ZTEST(basic, test_submit_delayed)
{
	struct workq q;
	struct k_sem sem;
	struct container *c;

	workq_init(&q);
	k_sem_init(&sem, 0, 3);

	for (size_t i = 0; i < 3; i++) {
		c = k_malloc(sizeof(struct container));
		zassert_not_null(c, "Failed to allocate memory for container");
		work_init(&c->item, work_fn);
		c->sem = &sem;
		zassert_true(workq_delayed_submit(&q, &c->item, K_MSEC(10 + i*10)) == 0,
				"workq_submit failed");
	}

	for (size_t i = 0; i < 3; i++) {
		zassert_ok(workq_run(&q, K_MSEC(50)), "workq_run failed");
		zassert_ok(k_sem_take(&sem, K_NO_WAIT), "work_fn not called");
	}
}

ZTEST(basic, test_cancel)
{
	struct workq q;
	struct k_sem sem;
	struct container *c;

	workq_init(&q);
	k_sem_init(&sem, 0, 1);

	c = k_malloc(sizeof(struct container));
	zassert_not_null(c, "Failed to allocate memory for container");
	c->sem = &sem;
	work_init(&c->item, work_fn);

	zassert_true(workq_delayed_submit(&q, &c->item, K_MSEC(50)) == 0, "workq_submit failed");
	zassert_ok(workq_cancel(&q, &c->item), "workq_cancel failed");
	zassert_true(workq_run(&q, K_MSEC(100)) == -EAGAIN, "workq_run should timeout");
	zassert_not_ok(k_sem_take(&sem, K_NO_WAIT), "work_fn called?");
}

static void work_fn_nonfree(struct work_item *item)
{
	LOG_DBG("%s called", __func__);
}

static struct k_sem started_sem;
static struct k_sem block_sem;

static void work_fn_block(struct work_item *item)
{
	k_sem_give(&started_sem);
	k_sem_take(&block_sem, K_SECONDS(1));
}

K_THREAD_STACK_DEFINE(stack, WORKQ_TEST_STACK_SIZE);
K_THREAD_STACK_DEFINE(busy_stack, WORKQ_TEST_STACK_SIZE);
ZTEST(basic, test_drain)
{
	struct workq_thread wqt;
	struct workq q;
	struct work_item item;
	struct work_item item2;

	workq_init(&q);
	work_init(&item, work_fn_nonfree);
	work_init(&item2, work_fn_nonfree);
	workq_thread_init(&wqt, &q, stack, K_THREAD_STACK_SIZEOF(stack), NULL);

	zassert_ok(workq_thread_start(&wqt), "workq_thread_start failed");
	zassert_ok(workq_delayed_submit(&q, &item, K_MSEC(500)), "workq_submit failed");
	zassert_true(-EAGAIN == workq_drain(&q, K_MSEC(200)), "workq_drain failed");
	zassert_ok(workq_drain(&q, K_MSEC(350)), "workq_drain failed");

	zassert_ok(workq_thread_stop(&wqt, K_MSEC(100)), "Failed to stop workq thread");
}

ZTEST(basic, test_reschedule)
{
	struct workq_thread wqt;
	struct work_item item;
	struct workq q;

	workq_init(&q);
	work_init(&item, work_fn_nonfree);
	workq_thread_init(&wqt, &q, stack, K_THREAD_STACK_SIZEOF(stack), NULL);

	zassert_ok(workq_thread_start(&wqt), "workq_thread_start failed");
	zassert_ok(workq_delayed_submit(&q, &item, K_MSEC(500)), "workq_submit failed");
	zassert_ok(workq_reschedule(&q, &item, K_MSEC(100)), "workq_reschedule failed");
	zassert_ok(workq_drain(&q, K_MSEC(200)), "workq_drain failed");

	zassert_ok(workq_thread_stop(&wqt, K_MSEC(100)), "Failed to stop workq thread");
}

ZTEST(basic, test_open_close)
{
	struct workq q;
	struct k_sem sem;
	struct container *c;

	k_sem_init(&sem, 0, 1);
	c = k_malloc(sizeof(struct container));
	zassert_not_null(c, "Failed to allocate memory for container");
	c->sem = &sem;
	work_init(&c->item, work_fn);

	workq_init(&q);

	workq_close(&q);
	zassert_true(-EAGAIN == workq_submit(&q, &c->item), "workq_submit should fail when closed");

	workq_open(&q);
	zassert_ok(workq_submit(&q, &c->item), "workq_open failed");
	zassert_ok(workq_run(&q, K_MSEC(100)), "workq_run should have successfully run the work");
	zassert_ok(k_sem_take(&sem, K_NO_WAIT), "work_fn not called");
}

ZTEST(basic, test_freeze_thaw)
{
	struct workq q;
	struct k_sem sem, sem2;
	struct container *c, *c2;

	k_sem_init(&sem, 0, 1);
	c = k_malloc(sizeof(struct container));
	zassert_not_null(c, "Failed to allocate memory for container");
	c->sem = &sem;
	work_init(&c->item, work_fn);

	workq_init(&q);
	zassert_ok(workq_delayed_submit(&q, &c->item, K_MSEC(50)), "workq_submit failed");
	workq_freeze(&q);

	/* Sleep a bit to make sure the work would(c) have been executed if it wasn't frozen */
	k_msleep(100);
	/* Sleep a bit more to make sure the work(c) would have been executed if it wasn't frozen */
	k_msleep(100);

	c2 = k_malloc(sizeof(struct container));
	zassert_not_null(c2, "Failed to allocate memory for container");
	k_sem_init(&sem2, 0, 1);
	c2->sem = &sem2;
	work_init(&c2->item, work_fn);
	zassert_ok(workq_submit(&q, &c2->item), "workq_submit failed");
	zassert_ok(workq_run(&q, K_MSEC(100)), "workq_run should have successfully run the work");
	zassert_ok(k_sem_take(&sem2, K_NO_WAIT), "work_fn not called");
	zassert_not_ok(k_sem_take(&sem, K_NO_WAIT),
		"work_fn should not have been called for frozen work");
	k_msleep(100);
	zassert_true(workq_submit(&q, &c->item) == -EALREADY,
			"work should already be submitted");
	workq_thaw(&q);
	zassert_ok(workq_run(&q, K_MSEC(10)), "workq_run should have successfully run the work");
	zassert_ok(k_sem_take(&sem, K_NO_WAIT), "work_fn not called");
}

ZTEST(basic, test_submit_already)
{
	struct workq q;
	struct work_item item;

	workq_init(&q);
	work_init(&item, work_fn_nonfree);

	zassert_ok(workq_submit(&q, &item), "first submit should succeed");
	zassert_equal(-EALREADY, workq_submit(&q, &item),
			"resubmit of a pending item should return -EALREADY");
	zassert_ok(workq_run(&q, K_NO_WAIT), "workq_run failed");
}

ZTEST(basic, test_cancel_enoent)
{
	struct workq q;
	struct work_item item;

	workq_init(&q);
	work_init(&item, work_fn_nonfree);

	zassert_equal(-ENOENT, workq_cancel(&q, &item),
			"cancel of an unknown item should return -ENOENT");
}

ZTEST(basic, test_cancel_busy)
{
	struct workq_thread wqt;
	struct workq q;
	struct work_item item;

	k_sem_init(&started_sem, 0, 1);
	k_sem_init(&block_sem, 0, 1);
	workq_init(&q);
	work_init(&item, work_fn_block);
	workq_thread_init(&wqt, &q, busy_stack, K_THREAD_STACK_SIZEOF(busy_stack), NULL);

	zassert_ok(workq_thread_start(&wqt), "workq_thread_start failed");
	zassert_ok(workq_submit(&q, &item), "workq_submit failed");
	zassert_ok(k_sem_take(&started_sem, K_MSEC(100)), "work did not start");

	zassert_equal(-EBUSY, workq_cancel(&q, &item),
			"cancel of an active item should return -EBUSY");

	k_sem_give(&block_sem);
	zassert_ok(workq_thread_stop(&wqt, K_MSEC(100)), "Failed to stop workq thread");
}

ZTEST(basic, test_closed_delayed_submit)
{
	struct workq q;
	struct work_item item;

	workq_init(&q);
	work_init(&item, work_fn_nonfree);
	workq_close(&q);

	zassert_equal(-EAGAIN, workq_delayed_submit(&q, &item, K_MSEC(10)),
			"workq_delayed_submit on a closed queue should return -EAGAIN");
	zassert_equal(-EAGAIN, workq_reschedule(&q, &item, K_MSEC(10)),
			"workq_reschedule on a closed queue should return -EAGAIN");
}

ZTEST(basic, test_thread_start_uninitialized)
{
	struct workq_thread wqt = {0};

	zassert_equal(-ENODEV, workq_thread_start(&wqt),
			"starting an uninitialized thread should return -ENODEV");
}

ZTEST(basic, test_stop_idle_worker)
{
	struct workq_thread wqt;
	struct workq q;

	workq_init(&q);
	workq_thread_init(&wqt, &q, stack, K_THREAD_STACK_SIZEOF(stack), NULL);

	zassert_ok(workq_thread_start(&wqt), "workq_thread_start failed");
	k_msleep(10); /* let the worker block waiting for work */

	zassert_ok(workq_thread_stop(&wqt, K_MSEC(100)),
			"stopping an idle worker should succeed");
	zassert_false(wqt.flags & WORKQ_THREAD_FLAG_RUNNING,
			"RUNNING flag should be cleared after stop");
}

ZTEST(basic, test_thaw_not_frozen)
{
	struct workq q;
	struct work_item item;

	workq_init(&q);
	work_init(&item, work_fn_nonfree);

	workq_thaw(&q);

	zassert_ok(workq_submit(&q, &item), "queue should stay usable after thaw");
	zassert_ok(workq_run(&q, K_NO_WAIT), "workq_run failed");
}

ZTEST_SUITE(basic, NULL, NULL, ztest_simple_1cpu_before,
	    ztest_simple_1cpu_after, NULL);
