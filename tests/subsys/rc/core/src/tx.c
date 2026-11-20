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

#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define IDLE_MS    20

static const struct device *const dev7 = DEVICE_DT_GET(DT_NODELABEL(rc_loopback7));
static const struct device *const dev10 = DEVICE_DT_GET(DT_NODELABEL(rc_loopback10));
static const struct device *const dev11 = DEVICE_DT_GET(DT_NODELABEL(rc_loopback11));

RC_DT_TX_IODEV(tx_iodev, DT_NODELABEL(rc_loopback10));
RC_DT_RAW_IODEV(tx_echo_iodev, DT_NODELABEL(rc_loopback10));
RTIO_DEFINE_WITH_MEMPOOL(tx_ctx, 4, 8, 4, 64, sizeof(uint32_t));

static const uint32_t frame[] = {
	RC_RAW_PULSE_US(9000), RC_RAW_SPACE_US(4500), RC_RAW_PULSE_US(560),
	RC_RAW_SPACE_US(1690), RC_RAW_PULSE_US(560),
};

static struct rtio_cqe *wait_cqe(struct rtio *r)
{
	struct rtio_cqe *cqe;

	for (int i = 0; i < 500; i++) {
		cqe = rtio_cqe_consume(r);
		if (cqe != NULL) {
			return cqe;
		}
		k_sleep(K_MSEC(1));
	}

	zassert_unreachable("no completion");

	return NULL;
}

static void expect_result(int result)
{
	struct rtio_cqe *cqe = wait_cqe(&tx_ctx);

	zassert_equal(cqe->result, result, "result %d", cqe->result);
	rtio_cqe_release(&tx_ctx, cqe);
	k_sleep(K_MSEC(IDLE_MS));
	zassert_is_null(rtio_cqe_consume(&tx_ctx), "second completion");
}

static int write_result(const uint8_t *buf, uint32_t len)
{
	struct rtio_sqe *sqe = rtio_sqe_acquire(&tx_ctx);
	struct rtio_cqe *cqe;
	int result;

	zassert_not_null(sqe);
	rtio_sqe_prep_write(sqe, &tx_iodev, RTIO_PRIO_NORM, buf, len, NULL);
	zassert_ok(rtio_submit(&tx_ctx, 0));
	cqe = wait_cqe(&tx_ctx);
	result = cqe->result;
	rtio_cqe_release(&tx_ctx, cqe);

	return result;
}

static void expect_sent(const uint32_t *words, size_t count, uint32_t hz, uint8_t duty)
{
	uint32_t got[CONFIG_RC_LOOPBACK_TX_WORDS];
	uint32_t got_hz;
	uint8_t got_duty;

	zassert_equal(rc_loopback_tx_get(dev10, got, ARRAY_SIZE(got), &got_hz, &got_duty), count);
	zassert_mem_equal(got, words, count * sizeof(words[0]));
	zassert_equal(got_hz, hz);
	zassert_equal(got_duty, duty);
}

static void rc_tx_before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_sleep(K_MSEC(IDLE_MS));
	zassert_ok(rc_set_tx_carrier(dev10, DT_PROP(DT_NODELABEL(rc_loopback10), carrier_hz),
				     DT_PROP(DT_NODELABEL(rc_loopback10), duty_cycle_percent)));
}

static void rc_tx_after(void *fixture)
{
	ARG_UNUSED(fixture);

	rc_loopback_tx_hold(dev10, false);
	zassert_ok(rc_rx_enable(dev10, false));
	/* RTIO frees a submission just after its completion */
	k_msleep(5);
	zassert_is_null(rtio_cqe_consume(&tx_ctx), "completion left");
	zassert_equal(tx_ctx.sqe_pool->pool_free, tx_ctx.sqe_pool->pool_size, "write leaked");
}

ZTEST(rc_tx, test_send_raw)
{
	uint32_t before = rc_loopback_tx_total(dev10);

	zassert_ok(rc_send_raw_sync(dev10, frame, ARRAY_SIZE(frame), K_MSEC(500)));
	zassert_equal(rc_loopback_tx_total(dev10), before + 1U);
	expect_sent(frame, ARRAY_SIZE(frame), 38000U, 50U);
}

ZTEST(rc_tx, test_carrier)
{
	zassert_ok(rc_set_tx_carrier(dev10, 36000U, 33U));
	zassert_ok(rc_send_raw_sync(dev10, frame, ARRAY_SIZE(frame), K_MSEC(500)));
	expect_sent(frame, ARRAY_SIZE(frame), 36000U, 33U);

	zassert_equal(rc_set_tx_carrier(dev10, 36000U, 0U), -EINVAL);
	zassert_equal(rc_set_tx_carrier(dev10, 36000U, 100U), -EINVAL);
	zassert_equal(rc_set_tx_carrier(dev10, 10000U, 50U), -EINVAL);
	zassert_equal(rc_set_tx_carrier(dev10, 70000U, 50U), -EINVAL);
	zassert_equal(rc_set_tx_carrier(dev11, 38000U, 50U), -ENOTSUP);
	zassert_equal(rc_send_raw_sync(dev11, frame, ARRAY_SIZE(frame), K_NO_WAIT), -ENOTSUP);
}

ZTEST(rc_tx, test_invalid_words)
{
	static uint32_t too_many[CONFIG_RC_LOOPBACK_TX_WORDS + 1];
	const uint32_t even[] = {RC_RAW_PULSE_US(100), RC_RAW_SPACE_US(100)};
	const uint32_t space_first[] = {RC_RAW_SPACE_US(100)};
	const uint32_t two_pulses[] = {RC_RAW_PULSE_US(100), RC_RAW_PULSE_US(100),
				       RC_RAW_PULSE_US(100)};
	const uint32_t zero[] = {RC_RAW_PULSE_US(100), RC_RAW_SPACE_US(0), RC_RAW_PULSE_US(100)};
	const uint32_t flagged[] = {RC_RAW_PULSE_US(100) | RC_RAW_TIMEOUT};
	const uint32_t too_long[] = {RC_RAW_PULSE_US(600000), RC_RAW_SPACE_US(1),
				     RC_RAW_PULSE_US(400000)};
	uint32_t before = rc_loopback_tx_total(dev10);

	for (size_t i = 0; i < ARRAY_SIZE(too_many); i++) {
		too_many[i] = ((i % 2U) == 0U) ? RC_RAW_PULSE_US(10) : RC_RAW_SPACE_US(10);
	}

	zassert_equal(rc_send_raw_sync(dev10, frame, 0, K_NO_WAIT), -EINVAL);
	zassert_equal(rc_send_raw_sync(dev10, even, ARRAY_SIZE(even), K_NO_WAIT), -EINVAL);
	zassert_equal(rc_send_raw_sync(dev10, space_first, 1, K_NO_WAIT), -EINVAL);
	zassert_equal(rc_send_raw_sync(dev10, two_pulses, 3, K_NO_WAIT), -EINVAL);
	zassert_equal(rc_send_raw_sync(dev10, zero, 3, K_NO_WAIT), -EINVAL);
	zassert_equal(rc_send_raw_sync(dev10, flagged, 1, K_NO_WAIT), -EINVAL);
	zassert_equal(rc_send_raw_sync(dev10, too_long, 3, K_NO_WAIT), -EINVAL);
	zassert_equal(rc_send_raw_sync(dev10, too_many, ARRAY_SIZE(too_many), K_NO_WAIT), -ENOBUFS);
	zassert_equal(write_result((const uint8_t *)too_many, sizeof(too_many)), -ENOBUFS);

	zassert_ok(rc_send_raw_sync(dev10,
				    (const uint32_t[]){RC_RAW_PULSE_US(500000), RC_RAW_SPACE_US(1),
						       RC_RAW_PULSE_US(499999)},
				    3, K_MSEC(500)));
	zassert_equal(rc_loopback_tx_total(dev10), before + 1U);
}

static K_THREAD_STACK_DEFINE(sender_stack, STACK_SIZE);
static struct k_thread sender;
static int sender_ret;

static void sender_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	sender_ret = rc_send_raw_sync(dev10, frame, ARRAY_SIZE(frame), K_FOREVER);
}

static void start_blocked_sender(void)
{
	rc_loopback_tx_hold(dev10, true);
	sender_ret = 1;
	k_thread_create(&sender, sender_stack, STACK_SIZE, sender_fn, NULL, NULL, NULL,
			K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	k_sleep(K_MSEC(IDLE_MS));
	zassert_equal(sender_ret, 1, "sender not blocked");
}

static void finish_blocked_sender(void)
{
	rc_loopback_tx_hold(dev10, false);
	zassert_ok(k_thread_join(&sender, K_MSEC(1000)));
	zassert_ok(sender_ret);
}

ZTEST(rc_tx, test_rx_not_stalled)
{
	const uint32_t word = RC_RAW_PULSE_US(100);
	struct rc_event evt;

	start_blocked_sender();

	zassert_ok(rc_rx_enable(dev7, true));
	k_sleep(K_MSEC(IDLE_MS));
	zassert_ok(rc_loopback_inject(dev7, &word, 1));
	zassert_ok(rc_receive_sync(dev7, &evt, K_MSEC(200)));
	zassert_equal(evt.scancode, 100U);
	zassert_ok(rc_rx_enable(dev7, false));

	finish_blocked_sender();
}

ZTEST(rc_tx, test_send_timeout_not_started)
{
	uint32_t before = rc_loopback_tx_total(dev10);

	start_blocked_sender();
	for (int i = 0; i < CONFIG_RC_SEND_SYNC_MAX + 1; i++) {
		zassert_equal(rc_send_raw_sync(dev10, frame, ARRAY_SIZE(frame), K_MSEC(20)),
			      -EAGAIN);
	}
	finish_blocked_sender();

	k_sleep(K_MSEC(IDLE_MS));
	zassert_equal(rc_loopback_tx_total(dev10), before + 1U);
}

static void release_timer_fn(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	rc_loopback_tx_hold(dev10, false);
}

static K_TIMER_DEFINE(release_timer, release_timer_fn, NULL);

ZTEST(rc_tx, test_send_timeout_while_sending)
{
	uint32_t before = rc_loopback_tx_total(dev10);

	rc_loopback_tx_hold(dev10, true);
	k_timer_start(&release_timer, K_MSEC(50), K_NO_WAIT);
	zassert_ok(rc_send_raw_sync(dev10, frame, ARRAY_SIZE(frame), K_MSEC(20)));
	zassert_equal(rc_loopback_tx_total(dev10), before + 1U);
}

ZTEST(rc_tx, test_tx_iodev)
{
	static const uint8_t odd_len[6];
	static uint8_t unaligned[sizeof(frame) + 1U];
	const uint32_t space_first[] = {RC_RAW_SPACE_US(100)};
	uint32_t before = rc_loopback_tx_total(dev10);
	uint32_t buf[4];
	struct rtio_sqe *sqe;
	struct rtio_cqe *cqe;

	zassert_ok(write_result((const uint8_t *)frame, sizeof(frame)));
	expect_sent(frame, ARRAY_SIZE(frame), 38000U, 50U);
	memcpy(&unaligned[1], frame, sizeof(frame));
	zassert_ok(write_result(&unaligned[1], sizeof(frame)));
	expect_sent(frame, ARRAY_SIZE(frame), 38000U, 50U);
	zassert_equal(write_result(odd_len, sizeof(odd_len)), -EINVAL);
	zassert_equal(write_result((const uint8_t *)space_first, sizeof(space_first)), -EINVAL);
	zassert_equal(rc_loopback_tx_total(dev10), before + 2U);

	sqe = rtio_sqe_acquire(&tx_ctx);
	zassert_not_null(sqe);
	rtio_sqe_prep_read(sqe, &tx_iodev, RTIO_PRIO_NORM, (uint8_t *)buf, sizeof(buf), NULL);
	zassert_ok(rtio_submit(&tx_ctx, 0));
	cqe = wait_cqe(&tx_ctx);
	zassert_equal(cqe->result, -EINVAL);
	rtio_cqe_release(&tx_ctx, cqe);
}

ZTEST(rc_tx, test_tx_iodev_cancel_queued)
{
	uint32_t before = rc_loopback_tx_total(dev10);
	struct rtio_sqe *sqe;
	rtio_sqe_handle_t queued;

	start_blocked_sender();

	sqe = rtio_sqe_acquire(&tx_ctx);
	zassert_not_null(sqe);
	rtio_sqe_prep_write(sqe, &tx_iodev, RTIO_PRIO_NORM, (const uint8_t *)frame, sizeof(frame),
			    NULL);
	queued = rtio_sqe_handle(&tx_ctx, sqe);
	zassert_ok(rtio_submit(&tx_ctx, 0));
	zassert_ok(rtio_sqe_cancel(&tx_ctx, queued));

	finish_blocked_sender();

	expect_result(-ECANCELED);
	zassert_equal(rc_loopback_tx_total(dev10), before + 1U);
}

ZTEST(rc_tx, test_tx_iodev_cancel_in_flight)
{
	uint32_t before = rc_loopback_tx_total(dev10);
	struct rtio_sqe *sqe;
	rtio_sqe_handle_t handle;

	rc_loopback_tx_hold(dev10, true);
	sqe = rtio_sqe_acquire(&tx_ctx);
	zassert_not_null(sqe);
	rtio_sqe_prep_write(sqe, &tx_iodev, RTIO_PRIO_NORM, (const uint8_t *)frame, sizeof(frame),
			    NULL);
	handle = rtio_sqe_handle(&tx_ctx, sqe);
	zassert_ok(rtio_submit(&tx_ctx, 0));
	k_sleep(K_MSEC(IDLE_MS));

	zassert_ok(rtio_sqe_cancel(&tx_ctx, handle));
	rc_loopback_tx_hold(dev10, false);
	expect_result(0);
	zassert_equal(rc_loopback_tx_total(dev10), before + 1U);
}

ZTEST(rc_tx, test_echo)
{
	rtio_sqe_handle_t handle = 0;
	uint32_t words[CONFIG_RC_RAW_CHUNK_WORDS];
	struct rtio_cqe *cqe;
	uint32_t len = 0U;
	uint8_t *buf = NULL;
	int result;

	zassert_ok(rc_rx_enable(dev10, true));
	zassert_ok(rc_stream(&tx_echo_iodev, &tx_ctx, NULL, &handle));
	zassert_ok(rc_send_raw_sync(dev10, frame, ARRAY_SIZE(frame), K_MSEC(500)));

	cqe = wait_cqe(&tx_ctx);
	zassert_equal(cqe->result, ARRAY_SIZE(frame));
	zassert_ok(rtio_cqe_get_mempool_buffer(&tx_ctx, cqe, &buf, &len));
	memcpy(words, buf, sizeof(frame));
	rtio_release_buffer(&tx_ctx, buf, len);
	rtio_cqe_release(&tx_ctx, cqe);
	zassert_mem_equal(words, frame, sizeof(frame));

	zassert_ok(rtio_sqe_cancel(&tx_ctx, handle));
	do {
		cqe = wait_cqe(&tx_ctx);
		result = cqe->result;
		len = 0U;
		buf = NULL;
		if ((rtio_cqe_get_mempool_buffer(&tx_ctx, cqe, &buf, &len) == 0) && (buf != NULL)) {
			rtio_release_buffer(&tx_ctx, buf, len);
		}
		rtio_cqe_release(&tx_ctx, cqe);
	} while (result != -ECANCELED);
}

ZTEST_SUITE(rc_tx, NULL, NULL, rc_tx_before, rc_tx_after, NULL);
