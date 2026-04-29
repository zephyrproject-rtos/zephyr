/*
 * Copyright (c) 2025 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/workq.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(multi_thread_tests, LOG_LEVEL_DBG);

struct container {
	struct work_item item;
};

static void work_fn(struct work_item *item)
{
	struct container *c = CONTAINER_OF(item, struct container, item);

	k_sleep(K_MSEC(100)); /* Simulate some work */
	k_free(c);
}

#define STACK_SIZE 1024
K_THREAD_STACK_ARRAY_DEFINE(thread_stacks, 3, STACK_SIZE);
static struct workq_thread wqts[3];

ZTEST(multi_thread, open_close)
{
	struct workq q;
	struct workq_thread wqt;

	workq_init(&q);
	workq_thread_init(&wqt, &q, thread_stacks[0], STACK_SIZE, NULL);
	zassert_ok(workq_thread_start(&wqt), "workq_thread_start failed");
	k_sleep(K_MSEC(50)); /* Give some time for thread to start */
	zassert_ok(workq_thread_stop(&wqt, K_FOREVER), "Failed to stop workq thread");
}

ZTEST(multi_thread, test_submit)
{
	struct workq q;
	struct container *c;

	workq_init(&q);

	for (size_t i = 0; i < 3; i++) {
		workq_thread_init(&wqts[i], &q, thread_stacks[i], STACK_SIZE, NULL);
		zassert_ok(workq_thread_start(&wqts[i]), "workq_thread_start failed");
	}

	for (size_t i = 0; i < 9; i++) {
		c = k_malloc(sizeof(struct container));
		zassert_not_null(c, "Failed to allocate memory for container");
		work_init(&c->item, work_fn);
		zassert_true(workq_submit(&q, &c->item) == 0, "workq_submit failed");
	}

	zassert_ok(workq_drain(&q, K_MSEC(350)), "workq_drain failed");
	for (size_t i = 0; i < 3; i++) {
		zassert_ok(workq_thread_stop(&wqts[i], K_MSEC(100)), "Failed to stop workq thread");
	}
}

ZTEST(multi_thread, test_close_one)
{
	struct workq q;
	struct container *c;

	workq_init(&q);

	for (size_t i = 0; i < 10; i++) {
		c = k_malloc(sizeof(struct container));
		zassert_not_null(c, "Failed to allocate memory for container");
		work_init(&c->item, work_fn);
		zassert_true(workq_submit(&q, &c->item) == 0, "workq_submit failed");
	}

	for (size_t i = 0; i < 3; i++) {
		workq_thread_init(&wqts[i], &q, thread_stacks[i], STACK_SIZE, NULL);
		zassert_ok(workq_thread_start(&wqts[i]), "workq_thread_start failed");
	}

	k_sleep(K_MSEC(50)); /* Give some time for threads to start */
	zassert_ok(workq_thread_stop(&wqts[0], K_MSEC(150)), "Failed to stop workq thread");

	/* The two remaining threads should finish the work */
	zassert_ok(workq_drain(&q, K_MSEC(500)), "workq_drain failed");
	for (size_t i = 0; i < 2; i++) {
		zassert_ok(workq_thread_stop(&wqts[i+1], K_MSEC(150)),
				"Failed to stop workq thread");
	}
}

ZTEST_SUITE(multi_thread, NULL, NULL, NULL, NULL, NULL);
