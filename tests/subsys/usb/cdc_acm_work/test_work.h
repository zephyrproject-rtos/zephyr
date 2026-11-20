/*
 * Copyright (c) 2026 Neoventor AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CDC_ACM_TEST_WORK_H_
#define CDC_ACM_TEST_WORK_H_

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

/* Only the driver under test uses this scheduler seam. Kernel buffer and
 * controller operations still execute their production implementations.
 */
struct test_work_slot {
	struct k_work_q *queue;
	struct k_work *work;
	int64_t due;
};

static struct test_work_slot work_slots[16];
static int64_t test_ticks;
static uint32_t test_work_runs;
static struct k_work *test_real_work;

static int test_schedule_work(struct k_work_q *queue, struct k_work *work,
			      k_timeout_t delay)
{
	struct test_work_slot *available = NULL;

	for (size_t i = 0; i < ARRAY_SIZE(work_slots); i++) {
		if (work_slots[i].work == work) {
			return 0;
		}
		if (available == NULL && work_slots[i].work == NULL) {
			available = &work_slots[i];
		}
	}

	zassert_not_null(available, "scheduler seam is full");
	available->queue = queue;
	available->work = work;
	available->due = test_ticks + delay.ticks;
	return 1;
}

static int test_submit_to_queue(struct k_work_q *queue, struct k_work *work)
{
	if (work == test_real_work) {
		return k_work_submit_to_queue(queue, work);
	}

	return test_schedule_work(queue, work, K_NO_WAIT);
}

static int test_schedule_for_queue(struct k_work_q *queue,
				   struct k_work_delayable *work, k_timeout_t delay)
{
	return test_schedule_work(queue, &work->work, delay);
}

static __unused int test_reschedule_for_queue(struct k_work_q *queue,
					     struct k_work_delayable *work, k_timeout_t delay)
{
	for (size_t i = 0; i < ARRAY_SIZE(work_slots); i++) {
		if (work_slots[i].work == &work->work) {
			work_slots[i].work = NULL;
		}
	}
	return test_schedule_work(queue, &work->work, delay);
}

static __unused int test_cancel(struct k_work *work)
{
	bool pending = false;

	if (work == test_real_work) {
		return k_work_cancel(work);
	}

	for (size_t i = 0; i < ARRAY_SIZE(work_slots); i++) {
		if (work_slots[i].work == work) {
			work_slots[i].work = NULL;
			pending = true;
		}
	}
	return pending;
}

static __unused bool test_cancel_sync(struct k_work *work, struct k_work_sync *sync)
{
	if (work == test_real_work) {
		return k_work_cancel_sync(work, sync);
	}
	return test_cancel(work) != 0;
}

static __unused bool test_cancel_delayable_sync(struct k_work_delayable *work,
					      struct k_work_sync *sync)
{
	return test_cancel_sync(&work->work, sync);
}

static int64_t test_work_delay(struct k_work *work)
{
	for (size_t i = 0; i < ARRAY_SIZE(work_slots); i++) {
		if (work_slots[i].work == work) {
			return work_slots[i].due - test_ticks;
		}
	}
	zassert_unreachable("expected pending work");
	return -1;
}

static bool test_work_pending(struct k_work *work)
{
	for (size_t i = 0; i < ARRAY_SIZE(work_slots); i++) {
		if (work_slots[i].work == work) {
			return true;
		}
	}
	return false;
}

/* Preserve the driver's queue-context checks during deterministic dispatch. */
static struct k_work *test_dispatched_work;

static void test_dispatch(struct k_work *work)
{
	ARG_UNUSED(work);
	test_dispatched_work->handler(test_dispatched_work);
}

static K_WORK_DEFINE(test_dispatch_work, test_dispatch);

static void test_run_work(struct k_work_q *queue, struct k_work *work)
{
	struct k_work_sync sync;

	test_dispatched_work = work;
	zassert_true(k_work_submit_to_queue(queue, &test_dispatch_work) >= 0);
	(void)k_work_flush(&test_dispatch_work, &sync);
}

static void test_advance(k_timeout_t delta)
{
	int64_t end = test_ticks + delta.ticks;

	for (size_t step = 0; step < 256; step++) {
		struct test_work_slot *next = NULL;
		struct k_work *work;

		for (size_t i = 0; i < ARRAY_SIZE(work_slots); i++) {
			if (work_slots[i].work != NULL && work_slots[i].due <= end &&
			    (next == NULL || work_slots[i].due < next->due)) {
				next = &work_slots[i];
			}
		}
		if (next == NULL) {
			test_ticks = end;
			return;
		}

		work = next->work;
		test_ticks = next->due;
		next->work = NULL;
		test_work_runs++;
		test_run_work(next->queue, work);
	}
	zassert_unreachable("driver work did not quiesce");
}

static void test_work_reset(void)
{
	memset(work_slots, 0, sizeof(work_slots));
	test_ticks = 0;
	test_work_runs = 0;
}

#endif /* CDC_ACM_TEST_WORK_H_ */
