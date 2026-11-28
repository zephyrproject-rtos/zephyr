/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "host/buf_waiter.h"

/* As many buffers as a test frees one by one, with a little data and user
 * data for a test to leave its traces in
 */
#define POOL_BUF_COUNT      3U
#define POOL_DATA_SIZE      4U
#define POOL_USER_DATA_SIZE 4U

/* More waiters than a test has waiting at a time */
#define WAITER_COUNT 3U

/* The blocked thread gets its buffer before the test goes on, as it outranks
 * the test's thread: the time only bounds a failure.
 */
#define THREAD_TIMEOUT K_SECONDS(1)

#define ALLOCATOR_STACK_SIZE 1024U

static void pool_destroy(struct net_buf *buf);

NET_BUF_POOL_FIXED_DEFINE(waiter_test_pool, POOL_BUF_COUNT, POOL_DATA_SIZE, POOL_USER_DATA_SIZE,
			  pool_destroy);

static BT_BUF_WAITERS_DEFINE(pool_waiters);

static void pool_destroy(struct net_buf *buf)
{
	bt_buf_waiters_destroy(&pool_waiters, buf);
}

/* A waiter, how often its handler has run, and the buffer that it got. The
 * handler only counts: the test calls bt_buf_waiter_alloc() itself, in the
 * order that it wants to check.
 */
struct test_waiter {
	struct bt_buf_waiter waiter;
	atomic_t resumed_count;
	struct net_buf *buf;
};

static struct test_waiter waiters[WAITER_COUNT];

/* The buffers that the test holds to keep the pool empty */
static struct net_buf *held[POOL_BUF_COUNT];

/* A thread that blocks in the allocator, and the buffer that it got */
static struct k_thread allocator_thread;
static atomic_t is_allocator_started;
static struct net_buf *allocator_buf;

static void resumed(struct k_work *work)
{
	struct bt_buf_waiter *waiter = CONTAINER_OF(work, struct bt_buf_waiter, work);
	struct test_waiter *tw = CONTAINER_OF(waiter, struct test_waiter, waiter);

	(void)atomic_inc(&tw->resumed_count);
}

static void allocator(void *p1, void *p2, void *p3)
{
	ARGS_UNUSED(p1, p2, p3);

	allocator_buf = net_buf_alloc(&waiter_test_pool, K_FOREVER);
}

/* Above the test's thread, so that it blocks in the allocator before the test
 * goes on
 */
static void allocator_start(void)
{
	static K_THREAD_STACK_DEFINE(allocator_stack, ALLOCATOR_STACK_SIZE);

	(void)atomic_set(&is_allocator_started, 1);
	(void)k_thread_create(&allocator_thread, allocator_stack,
			      K_THREAD_STACK_SIZEOF(allocator_stack), allocator, NULL, NULL, NULL,
			      K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
}

static void pool_hold(void)
{
	ARRAY_FOR_EACH(held, i) {
		held[i] = net_buf_alloc(&waiter_test_pool, K_NO_WAIT);
		zassert_not_null(held[i], "The pool has %zu free buffers", i);
	}
}

/* The handlers of the waiters that a call resumes run before the test goes
 * on: the workqueue outranks the test's thread, but it only gets to run when
 * that thread next gives way.
 */
static void waiter_alloc(struct test_waiter *tw)
{
	tw->buf = bt_buf_waiter_alloc(&pool_waiters, &tw->waiter, &waiter_test_pool);
	k_yield();
}

static void buf_free(struct net_buf **buf)
{
	net_buf_drop(buf);
	k_yield();
}

static void waiter_cancel(struct test_waiter *tw)
{
	bt_buf_waiter_cancel(&pool_waiters, &tw->waiter);
	k_yield();
}

static void *setup(void)
{
	/* The handlers of the waiters run on the Host's workqueue */
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");

	ARRAY_FOR_EACH_PTR(waiters, tw) {
		bt_buf_waiter_init(&tw->waiter, resumed);
	}

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	ARRAY_FOR_EACH_PTR(waiters, tw) {
		(void)atomic_clear(&tw->resumed_count);
	}
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	ARRAY_FOR_EACH_PTR(waiters, tw) {
		bt_buf_waiter_cancel(&pool_waiters, &tw->waiter);
		net_buf_drop(&tw->buf);
	}

	ARRAY_FOR_EACH(held, i) {
		net_buf_drop(&held[i]);
	}

	/* With every buffer back, a thread that is still blocked gets one */
	if (atomic_cas(&is_allocator_started, 1, 0)) {
		zassert_ok(k_thread_join(&allocator_thread, THREAD_TIMEOUT),
			   "The allocating thread is still blocked");
	}

	net_buf_drop(&allocator_buf);

	zassert_equal(atomic_get(&waiter_test_pool.avail_count), POOL_BUF_COUNT,
		      "%ld of the pool's buffers are free",
		      atomic_get(&waiter_test_pool.avail_count));
}

static void teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(bt_disable(), "Bluetooth disable failed");
}

ZTEST_SUITE(buf_waiter, NULL, setup, before, after, teardown);

/* With a buffer in the pool the waiter gets it and does not wait */
static ZTEST(buf_waiter, test_pool_has_buffer)
{
	waiter_alloc(&waiters[0]);
	zassert_not_null(waiters[0].buf, "No buffer from a full pool");

	net_buf_drop(&waiters[0].buf);

	zassert_equal(atomic_get(&waiter_test_pool.avail_count), POOL_BUF_COUNT,
		      "The freed buffer was kept for a waiter that has one");
	zassert_equal(atomic_get(&waiters[0].resumed_count), 0, "The waiter was resumed");
}

/* A freed buffer is kept for the waiter that has waited longest, which is
 * resumed and gets it as a new buffer. The next one goes to the pool, and the
 * next waiter is resumed to take it from there.
 */
static ZTEST(buf_waiter, test_waiters_in_order)
{
	static const uint8_t no_user_data[POOL_USER_DATA_SIZE];

	pool_hold();

	waiter_alloc(&waiters[0]);
	waiter_alloc(&waiters[1]);
	zassert_is_null(waiters[0].buf, "A buffer from an empty pool");
	zassert_is_null(waiters[1].buf, "A buffer from an empty pool");

	/* What an allocation would not leave in the buffer */
	(void)net_buf_add_u8(held[0], 0xa5U);
	(void)memset(net_buf_user_data(held[0]), 0xa5, POOL_USER_DATA_SIZE);
	buf_free(&held[0]);

	zassert_equal(atomic_get(&waiter_test_pool.avail_count), 0,
		      "The freed buffer went to the pool");
	zassert_equal(atomic_get(&waiters[0].resumed_count), 1,
		      "The first waiter was resumed %ld times",
		      atomic_get(&waiters[0].resumed_count));

	waiter_alloc(&waiters[1]);
	zassert_is_null(waiters[1].buf, "The second waiter got the buffer");
	waiter_alloc(&waiters[0]);
	zassert_not_null(waiters[0].buf, "The first waiter did not get the buffer");
	zassert_equal(waiters[0].buf->ref, 1U, "Reference count %u", waiters[0].buf->ref);
	zassert_equal(waiters[0].buf->len, 0U, "%u bytes of data left", waiters[0].buf->len);
	zassert_mem_equal(net_buf_user_data(waiters[0].buf), no_user_data, POOL_USER_DATA_SIZE,
			  "User data left");

	(void)atomic_clear(&waiters[1].resumed_count);
	buf_free(&held[1]);

	zassert_equal(atomic_get(&waiter_test_pool.avail_count), 1,
		      "The buffer after a kept one did not go to the pool");
	zassert_equal(atomic_get(&waiters[1].resumed_count), 1,
		      "The second waiter was resumed %ld times",
		      atomic_get(&waiters[1].resumed_count));

	waiter_alloc(&waiters[1]);
	zassert_not_null(waiters[1].buf, "The second waiter did not get the buffer");
}

/* The freed buffers alternate between the waiters and a thread that is
 * blocked in the allocator
 */
static ZTEST(buf_waiter, test_waiters_and_blocked_thread)
{
	pool_hold();

	waiter_alloc(&waiters[0]);
	waiter_alloc(&waiters[1]);
	allocator_start();
	zassert_is_null(allocator_buf, "The thread got a buffer from an empty pool");

	buf_free(&held[0]);

	zassert_is_null(allocator_buf, "The thread got the first buffer");
	waiter_alloc(&waiters[0]);
	zassert_not_null(waiters[0].buf, "The first waiter did not get the first buffer");

	buf_free(&held[1]);

	zassert_ok(k_thread_join(&allocator_thread, THREAD_TIMEOUT),
		   "The thread did not get the second buffer");
	(void)atomic_clear(&is_allocator_started);
	zassert_not_null(allocator_buf, "The thread did not get the second buffer");
	waiter_alloc(&waiters[1]);
	zassert_is_null(waiters[1].buf, "The second waiter got the second buffer");

	buf_free(&held[2]);

	waiter_alloc(&waiters[1]);
	zassert_not_null(waiters[1].buf, "The second waiter did not get the third buffer");
}

/* A buffer in the pool is left to the waiter that has waited longest. When
 * that one has it, the next waiter is resumed.
 */
static ZTEST(buf_waiter, test_pool_left_to_first_waiter)
{
	pool_hold();

	waiter_alloc(&waiters[0]);
	waiter_alloc(&waiters[1]);

	/* Kept for the first waiter, then one to the pool for the second */
	buf_free(&held[0]);
	buf_free(&held[1]);
	zassert_equal(atomic_get(&waiter_test_pool.avail_count), 1, "No buffer in the pool");

	waiter_alloc(&waiters[2]);
	zassert_is_null(waiters[2].buf, "A new waiter got the buffer of one that waits");
	zassert_equal(atomic_get(&waiters[2].resumed_count), 0, "The new waiter was resumed");

	waiter_alloc(&waiters[1]);
	zassert_not_null(waiters[1].buf, "The second waiter did not get the buffer in the pool");
	zassert_equal(atomic_get(&waiters[2].resumed_count), 1,
		      "The waiter that is first now was resumed %ld times",
		      atomic_get(&waiters[2].resumed_count));

	waiter_alloc(&waiters[0]);
	zassert_not_null(waiters[0].buf, "The first waiter did not get the buffer kept for it");
}

/* A waiter that is cancelled while it is first leaves the buffer in the pool
 * to the next one, which is resumed
 */
static ZTEST(buf_waiter, test_cancel_resumes_next_waiter)
{
	pool_hold();

	waiter_alloc(&waiters[0]);
	waiter_alloc(&waiters[1]);
	waiter_alloc(&waiters[2]);

	/* Kept for the first waiter, then one to the pool for the second */
	buf_free(&held[0]);
	buf_free(&held[1]);
	zassert_equal(atomic_get(&waiters[2].resumed_count), 0, "The third waiter was resumed");

	waiter_cancel(&waiters[1]);

	zassert_equal(atomic_get(&waiters[2].resumed_count), 1,
		      "The third waiter was resumed %ld times",
		      atomic_get(&waiters[2].resumed_count));
	waiter_alloc(&waiters[2]);
	zassert_not_null(waiters[2].buf, "The third waiter did not get the buffer in the pool");
}

/* A buffer that was kept for a waiter goes back when the waiter is cancelled */
static ZTEST(buf_waiter, test_cancel_frees_kept_buffer)
{
	pool_hold();

	waiter_alloc(&waiters[0]);
	buf_free(&held[0]);
	zassert_equal(atomic_get(&waiter_test_pool.avail_count), 0,
		      "The freed buffer was not kept for the waiter");

	waiter_cancel(&waiters[0]);

	zassert_equal(atomic_get(&waiter_test_pool.avail_count), 1,
		      "The kept buffer did not go back to the pool");

	/* Nothing is kept for a waiter that has left */
	buf_free(&held[1]);
	zassert_equal(atomic_get(&waiter_test_pool.avail_count), 2,
		      "A buffer was kept for the cancelled waiter");
}
