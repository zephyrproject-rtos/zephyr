/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/rc.h>
#include <zephyr/drivers/rc/rc_loopback.h>
#include <zephyr/kernel.h>
#include <zephyr/rc/rc.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/ztest.h>

#define TEST_PROTO_EVT 0x1fa
#define KEYUP_MS       6
#define WAITERS        CONFIG_RC_RECEIVE_SYNC_MAX
#define STACK_SIZE     (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

static const struct device *const dev7 = DEVICE_DT_GET(DT_NODELABEL(rc_loopback7));

RC_DT_EVENT_IODEV(sync_iodev, DT_NODELABEL(rc_loopback7));
RTIO_DEFINE_WITH_MEMPOOL(sync_ctx, 4, 8, 8, 32, sizeof(void *));

static uint32_t timer_scancode;

static void inject_timer_fn(struct k_timer *timer)
{
	const uint32_t word = RC_RAW_PULSE_US(timer_scancode);

	ARG_UNUSED(timer);

	(void)rc_loopback_inject(dev7, &word, 1);
}

static K_TIMER_DEFINE(inject_timer, inject_timer_fn, NULL);

static void inject_later(uint32_t scancode, int ms)
{
	timer_scancode = scancode;
	k_timer_start(&inject_timer, K_MSEC(ms), K_NO_WAIT);
}

static void rc_sync_before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_sleep(K_MSEC(4 * KEYUP_MS));
	zassert_ok(rc_rx_enable(dev7, true));
}

static void rc_sync_after(void *fixture)
{
	ARG_UNUSED(fixture);

	k_timer_stop(&inject_timer);
	zassert_ok(rc_rx_enable(dev7, false));
}

ZTEST(rc_sync, test_event)
{
	struct rc_event evt;

	inject_later(100, 10);
	zassert_ok(rc_receive_sync(dev7, &evt, K_MSEC(500)));
	zassert_equal(evt.proto, TEST_PROTO_EVT);
	zassert_equal(evt.scancode, 100U);
	zassert_equal(evt.flags, 0);
}

ZTEST(rc_sync, test_release)
{
	struct rc_event evt;

	inject_later(100, 10);
	zassert_ok(rc_receive_sync(dev7, &evt, K_MSEC(500)));
	zassert_equal(evt.flags, 0);

	zassert_ok(rc_receive_sync(dev7, &evt, K_MSEC(500)));
	zassert_equal(evt.scancode, 100U);
	zassert_equal(evt.flags, RC_EVENT_FLAG_RELEASE);
}

ZTEST(rc_sync, test_timeout_no_leak)
{
	struct rc_event evt;

	for (int i = 0; i < 2 * WAITERS + 1; i++) {
		zassert_equal(rc_receive_sync(dev7, &evt, K_MSEC(5)), -EAGAIN);
		zassert_equal(rc_receive_sync(dev7, &evt, K_NO_WAIT), -EAGAIN);
	}

	inject_later(100, 10);
	zassert_ok(rc_receive_sync(dev7, &evt, K_MSEC(500)));
	zassert_equal(evt.scancode, 100U);
}

ZTEST(rc_sync, test_with_stream)
{
	rtio_sqe_handle_t handle = 0;
	struct rtio_cqe *cqe;
	struct rc_event evt, streamed;
	uint32_t len = 0U;
	uint8_t *buf = NULL;
	int result;

	zassert_ok(rc_stream(&sync_iodev, &sync_ctx, NULL, &handle));
	inject_later(100, 10);
	zassert_ok(rc_receive_sync(dev7, &evt, K_MSEC(500)));
	zassert_equal(evt.scancode, 100U);

	cqe = rtio_cqe_consume_block(&sync_ctx);
	zassert_equal(cqe->result, 0);
	zassert_ok(rtio_cqe_get_mempool_buffer(&sync_ctx, cqe, &buf, &len));
	memcpy(&streamed, buf, sizeof(streamed));
	rtio_release_buffer(&sync_ctx, buf, len);
	rtio_cqe_release(&sync_ctx, cqe);
	zassert_mem_equal(&streamed, &evt, sizeof(evt));

	k_sleep(K_MSEC(4 * KEYUP_MS));
	zassert_ok(rtio_sqe_cancel(&sync_ctx, handle));
	do {
		cqe = rtio_cqe_consume_block(&sync_ctx);
		len = 0U;
		buf = NULL;
		if ((rtio_cqe_get_mempool_buffer(&sync_ctx, cqe, &buf, &len) == 0) &&
		    (buf != NULL)) {
			rtio_release_buffer(&sync_ctx, buf, len);
		}
		result = cqe->result;
		rtio_cqe_release(&sync_ctx, cqe);
	} while (result != -ECANCELED);
	zassert_equal(sync_ctx.sqe_pool->pool_free, sync_ctx.sqe_pool->pool_size);
}

static K_THREAD_STACK_ARRAY_DEFINE(waiter_stacks, WAITERS, STACK_SIZE);
static struct k_thread waiters[WAITERS];
static struct rc_event waiter_evt[WAITERS];
static int waiter_ret[WAITERS];

static void waiter_fn(void *p1, void *p2, void *p3)
{
	int i = POINTER_TO_INT(p1);

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	waiter_ret[i] = rc_receive_sync(dev7, &waiter_evt[i], K_MSEC(500));
}

ZTEST(rc_sync, test_concurrent_waiters)
{
	struct rc_event evt;

	for (int i = 0; i < WAITERS; i++) {
		k_thread_create(&waiters[i], waiter_stacks[i], STACK_SIZE, waiter_fn,
				INT_TO_POINTER(i), NULL, NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	}
	k_sleep(K_MSEC(10));

	zassert_equal(rc_receive_sync(dev7, &evt, K_NO_WAIT), -ENOMEM);

	inject_later(100, 5);
	for (int i = 0; i < WAITERS; i++) {
		zassert_ok(k_thread_join(&waiters[i], K_MSEC(1000)));
		zassert_ok(waiter_ret[i]);
		zassert_equal(waiter_evt[i].scancode, 100U);
	}
}

ZTEST_SUITE(rc_sync, NULL, NULL, rc_sync_before, rc_sync_after, NULL);
