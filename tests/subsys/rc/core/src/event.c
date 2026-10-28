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
#include <zephyr/rc/rc_protocol.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/ztest.h>

#define TEST_PROTO_EVT 0x1fa

static int test_evt_decode(void *state, uint32_t word, struct rc_decode_result *res)
{
	ARG_UNUSED(state);

	if (!rc_raw_is_pulse(word)) {
		return 0;
	}

	res->proto = TEST_PROTO_EVT;
	res->flags = 0;
	res->scancode = rc_raw_duration_us(word);

	return 1;
}

RC_PROTOCOL_DEFINE(test_proto_evt) = {
	.name = "test-evt",
	.proto_first = TEST_PROTO_EVT,
	.proto_last = TEST_PROTO_EVT,
	.decode = test_evt_decode,
};

RC_DT_EVENT_IODEV(evt_iodev, DT_NODELABEL(rc_loopback5));

RTIO_DEFINE_WITH_MEMPOOL(ctx_a, 4, 8, 8, 32, sizeof(void *));
RTIO_DEFINE_WITH_MEMPOOL(ctx_b, 4, 8, 8, 32, sizeof(void *));
RTIO_DEFINE_WITH_MEMPOOL(ctx_small, 4, 8, 1, 8, sizeof(void *));
RTIO_DEFINE(ctx_plain, 4, 8);

static const struct device *const dev5 = DEVICE_DT_GET(DT_NODELABEL(rc_loopback5));

static void inject(uint32_t scancode)
{
	const uint32_t word = RC_RAW_PULSE_US(scancode);

	zassert_ok(rc_loopback_inject(dev5, &word, 1));
}

static struct rtio_cqe *wait_cqe(struct rtio *r)
{
	for (int i = 0; i < 200; i++) {
		struct rtio_cqe *cqe = rtio_cqe_consume(r);

		if (cqe != NULL) {
			return cqe;
		}
		k_sleep(K_MSEC(1));
	}

	zassert_unreachable("no completion");

	return NULL;
}

static void expect_no_cqe(struct rtio *r)
{
	k_sleep(K_MSEC(20));
	zassert_is_null(rtio_cqe_consume(r), "unexpected completion");
}

static void expect_event(struct rtio *r, void *userdata, uint32_t scancode, struct rc_event *out)
{
	struct rtio_cqe *cqe = wait_cqe(r);
	struct rc_event evt;
	uint32_t len = 0U;
	uint8_t *buf = NULL;

	zassert_equal(cqe->result, 0);
	zassert_equal(cqe->userdata, userdata);
	zassert_ok(rtio_cqe_get_mempool_buffer(r, cqe, &buf, &len));
	zassert_true(len >= sizeof(evt));
	memcpy(&evt, buf, sizeof(evt));
	rtio_release_buffer(r, buf, len);
	rtio_cqe_release(r, cqe);

	zassert_equal(evt.proto, TEST_PROTO_EVT);
	zassert_equal(evt.scancode, scancode);
	zassert_equal(evt.flags, 0);
	zassert_equal(evt.keycode, 0);
	if (out != NULL) {
		*out = evt;
	}
}

static void expect_result(struct rtio *r, int result)
{
	struct rtio_cqe *cqe = wait_cqe(r);

	zassert_equal(cqe->result, result, "result %d", cqe->result);
	rtio_cqe_release(r, cqe);
}

static void cancel(struct rtio *r, rtio_sqe_handle_t handle)
{
	zassert_ok(rtio_sqe_cancel(r, handle));
	expect_result(r, -ECANCELED);
}

static void cancel_before_submit(struct rtio *r, struct rtio_sqe *sqe)
{
	zassert_ok(rtio_sqe_cancel(r, rtio_sqe_handle(r, sqe)));
	zassert_ok(rtio_submit(r, 0));
	expect_result(r, -ECANCELED);
	expect_no_cqe(r);
}

static void submit_oneshot(struct rtio *r, void *userdata)
{
	struct rtio_sqe *sqe = rtio_sqe_acquire(r);

	zassert_not_null(sqe);
	rtio_sqe_prep_read_with_pool(sqe, &evt_iodev, RTIO_PRIO_NORM, userdata);
	zassert_ok(rtio_submit(r, 0));
}

static void rc_event_before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(rc_rx_enable(dev5, true));
}

static void rc_event_after(void *fixture)
{
	struct rtio *const ctxs[] = {&ctx_a, &ctx_b, &ctx_small, &ctx_plain};

	ARG_UNUSED(fixture);

	zassert_ok(rc_rx_enable(dev5, false));
	k_sleep(K_MSEC(5));
	ARRAY_FOR_EACH(ctxs, i) {
		zassert_is_null(rtio_cqe_consume(ctxs[i]), "completion left in ctx %zu", i);
		zassert_equal(ctxs[i]->sqe_pool->pool_free, ctxs[i]->sqe_pool->pool_size,
			      "submission leaked in ctx %zu", i);
		if (ctxs[i]->block_pool != NULL) {
			zassert_equal(ctxs[i]->block_pool->info.used_blocks, 0U,
				      "pool buffer leaked in ctx %zu", i);
		}
	}
}

ZTEST(rc_event, test_multishot_every_event)
{
	rtio_sqe_handle_t handle;
	struct rc_event first, second;

	zassert_ok(rc_stream(&evt_iodev, &ctx_a, &ctx_a, &handle));
	inject(100);
	inject(200);
	inject(300);

	expect_event(&ctx_a, &ctx_a, 100, &first);
	expect_event(&ctx_a, &ctx_a, 200, &second);
	expect_event(&ctx_a, &ctx_a, 300, NULL);
	zassert_true(first.timestamp_ns > 0U);
	zassert_true(second.timestamp_ns >= first.timestamp_ns);

	cancel(&ctx_a, handle);
	inject(400);
	expect_no_cqe(&ctx_a);
}

ZTEST(rc_event, test_oneshot_once)
{
	submit_oneshot(&ctx_a, NULL);
	inject(100);
	inject(200);

	expect_event(&ctx_a, NULL, 100, NULL);
	expect_no_cqe(&ctx_a);
}

ZTEST(rc_event, test_no_submission_dropped)
{
	inject(100);
	k_sleep(K_MSEC(5));

	submit_oneshot(&ctx_a, NULL);
	inject(200);
	expect_event(&ctx_a, NULL, 200, NULL);
}

ZTEST(rc_event, test_two_contexts)
{
	rtio_sqe_handle_t handle_a, handle_b;

	zassert_ok(rc_stream(&evt_iodev, &ctx_a, NULL, &handle_a));
	zassert_ok(rc_stream(&evt_iodev, &ctx_b, NULL, &handle_b));
	inject(100);

	expect_event(&ctx_a, NULL, 100, NULL);
	expect_event(&ctx_b, NULL, 100, NULL);

	cancel(&ctx_a, handle_a);
	cancel(&ctx_b, handle_b);
}

ZTEST(rc_event, test_cancel_without_events)
{
	rtio_sqe_handle_t handle;

	zassert_ok(rc_stream(&evt_iodev, &ctx_a, NULL, &handle));
	k_sleep(K_MSEC(5));
	zassert_equal(ctx_a.sqe_pool->pool_free, ctx_a.sqe_pool->pool_size - 1U);

	cancel(&ctx_a, handle);
	zassert_equal(ctx_a.sqe_pool->pool_free, ctx_a.sqe_pool->pool_size);

	inject(100);
	expect_no_cqe(&ctx_a);
}

ZTEST(rc_event, test_cancel_twice)
{
	rtio_sqe_handle_t handle;

	zassert_ok(rc_stream(&evt_iodev, &ctx_a, NULL, &handle));
	cancel(&ctx_a, handle);

	zassert_ok(rtio_sqe_cancel(&ctx_a, handle));
	expect_no_cqe(&ctx_a);
}

ZTEST(rc_event, test_cancel_oneshot)
{
	struct rtio_sqe *sqe = rtio_sqe_acquire(&ctx_a);
	rtio_sqe_handle_t handle;

	zassert_not_null(sqe);
	rtio_sqe_prep_read_with_pool(sqe, &evt_iodev, RTIO_PRIO_NORM, NULL);
	handle = rtio_sqe_handle(&ctx_a, sqe);
	zassert_ok(rtio_submit(&ctx_a, 0));
	k_sleep(K_MSEC(5));

	cancel(&ctx_a, handle);
	inject(100);
	expect_no_cqe(&ctx_a);
}

ZTEST(rc_event, test_cancel_before_submit)
{
	struct rtio_sqe *sqe = rtio_sqe_acquire(&ctx_a);

	zassert_not_null(sqe);
	rtio_sqe_prep_read_with_pool(sqe, &evt_iodev, RTIO_PRIO_NORM, NULL);
	cancel_before_submit(&ctx_a, sqe);
}

ZTEST(rc_event, test_pool_exhausted)
{
	rtio_sqe_handle_t handle;

	zassert_ok(rc_stream(&evt_iodev, &ctx_small, NULL, &handle));
	inject(100);
	expect_result(&ctx_small, -ENOMEM);

	inject(200);
	expect_no_cqe(&ctx_small);
}

ZTEST(rc_event, test_app_buffer)
{
	struct rc_event evt;
	uint8_t small[sizeof(evt) - 1];
	struct rtio_sqe *sqe;

	sqe = rtio_sqe_acquire(&ctx_plain);
	zassert_not_null(sqe);
	rtio_sqe_prep_read(sqe, &evt_iodev, RTIO_PRIO_NORM, (uint8_t *)&evt, sizeof(evt), NULL);
	zassert_ok(rtio_submit(&ctx_plain, 0));
	inject(100);
	expect_result(&ctx_plain, 0);
	zassert_equal(evt.scancode, 100U);

	sqe = rtio_sqe_acquire(&ctx_plain);
	zassert_not_null(sqe);
	rtio_sqe_prep_read(sqe, &evt_iodev, RTIO_PRIO_NORM, small, sizeof(small), NULL);
	zassert_ok(rtio_submit(&ctx_plain, 0));
	inject(100);
	expect_result(&ctx_plain, -ENOMEM);
}

ZTEST(rc_event, test_invalid_op)
{
	static const uint8_t data[4];
	struct rtio_sqe *sqe = rtio_sqe_acquire(&ctx_plain);

	zassert_not_null(sqe);
	rtio_sqe_prep_write(sqe, &evt_iodev, RTIO_PRIO_NORM, data, sizeof(data), NULL);
	zassert_ok(rtio_submit(&ctx_plain, 0));
	expect_result(&ctx_plain, -EINVAL);
}

ZTEST_SUITE(rc_event, NULL, NULL, rc_event_before, rc_event_after, NULL);
