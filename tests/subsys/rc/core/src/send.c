/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/rc.h>
#include <zephyr/drivers/rc/rc_loopback.h>
#include <zephyr/kernel.h>
#include <zephyr/rc/rc.h>
#include <zephyr/rc/rc_protocol.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/ztest.h>

#define TEST_PROTO_TX_REP  0x1f4
#define TEST_PROTO_TX_TOG  0x1f5
#define TEST_PROTO_TX_BAD  0x1f6
#define TEST_PROTO_TX_FAR  0x1f7
#define TEST_PROTO_TX_LONG 0x1f9
#define TEST_PROTO_TX_RT   0x1fd
#define TEST_PROTO_NONE    0x1fe
#define TEST_PROTO_KEY_A   0x1fb

#define REP_PERIOD_MS  100
#define TOG_PERIOD_MS  50
#define LONG_PERIOD_MS 50
#define LONG_GAP_MS    20
#define LONG_FRAME_MS  60
#define RT_PERIOD_MS   30
#define RT_REPEAT_MS   100
#define ENCODE_MS      40
#define SLACK_MS       40
#define TOG_BIT        0x1000U
#define REPEAT_US      9000U
#define LOG_SIZE       8

BUILD_ASSERT(CONFIG_RC_TX_MAX_DURATION_MS == 500);

static const struct device *const dev10 = DEVICE_DT_GET(DT_NODELABEL(rc_loopback10));
static const struct device *const dev11 = DEVICE_DT_GET(DT_NODELABEL(rc_loopback11));
static const struct device *const dev12 = DEVICE_DT_GET(DT_NODELABEL(rc_loopback12));
static const struct device *const dev13 = DEVICE_DT_GET(DT_NODELABEL(rc_loopback13));
static const struct device *const dev14 = DEVICE_DT_GET(DT_NODELABEL(rc_loopback14));

RC_DT_TX_IODEV(send_iodev, DT_NODELABEL(rc_loopback12));
RTIO_DEFINE(send_ctx, 2, 2);

RC_DT_EVENT_IODEV(rt_iodev, DT_NODELABEL(rc_loopback13));
RTIO_DEFINE_WITH_MEMPOOL(rt_ctx, 4, 16, 16, 32, sizeof(void *));

static struct {
	bool repeat;
	uint16_t flags;
	size_t max;
} enc_log[LOG_SIZE];
static size_t enc_count;
static int64_t tx_ticks[LOG_SIZE];
static size_t tx_count;

static void enc_record(bool repeat, uint16_t flags, size_t max)
{
	if (enc_count < LOG_SIZE) {
		enc_log[enc_count].repeat = repeat;
		enc_log[enc_count].flags = flags;
		enc_log[enc_count].max = max;
	}
	enc_count++;
}

static void tx_record(const struct device *dev)
{
	ARG_UNUSED(dev);

	if (tx_count < LOG_SIZE) {
		tx_ticks[tx_count] = k_uptime_ticks();
	}
	tx_count++;
}

static int rep_encode(uint16_t proto, uint32_t scancode, uint16_t flags, uint32_t *words,
		      size_t max)
{
	ARG_UNUSED(proto);

	enc_record(false, flags, max);
	if (scancode > 0xfffU) {
		return -EINVAL;
	}
	words[0] = RC_RAW_PULSE_US(1000U + scancode);
	words[1] = RC_RAW_SPACE_US(500U);
	words[2] = RC_RAW_PULSE_US(500U);

	return 3;
}

static int rep_encode_repeat(uint16_t proto, uint32_t scancode, uint16_t flags, uint32_t *words,
			     size_t max)
{
	ARG_UNUSED(proto);
	ARG_UNUSED(scancode);

	enc_record(true, flags, max);
	k_msleep(ENCODE_MS);
	words[0] = RC_RAW_PULSE_US(REPEAT_US);

	return 1;
}

RC_PROTOCOL_DEFINE(test_proto_tx_rep) = {
	.name = "test-tx-rep",
	.proto_first = TEST_PROTO_TX_REP,
	.proto_last = TEST_PROTO_TX_REP,
	.carrier_hz = 36000U,
	.tx_period_ms = REP_PERIOD_MS,
	.encode = rep_encode,
	.encode_repeat = rep_encode_repeat,
};

static int tog_encode(uint16_t proto, uint32_t scancode, uint16_t flags, uint32_t *words,
		      size_t max)
{
	ARG_UNUSED(proto);

	enc_record(false, flags, max);
	words[0] = RC_RAW_PULSE_US(2000U + scancode +
				   (((flags & RC_EVENT_FLAG_TOGGLE) != 0U) ? TOG_BIT : 0U));

	return 1;
}

RC_PROTOCOL_DEFINE(test_proto_tx_tog) = {
	.name = "test-tx-tog",
	.proto_first = TEST_PROTO_TX_TOG,
	.proto_last = TEST_PROTO_TX_TOG,
	.tx_period_ms = TOG_PERIOD_MS,
	.encode = tog_encode,
};

static int bad_encode(uint16_t proto, uint32_t scancode, uint16_t flags, uint32_t *words,
		      size_t max)
{
	ARG_UNUSED(proto);

	enc_record(false, flags, max);
	switch (scancode) {
	case 2U:
		return (max < (CONFIG_RC_LOOPBACK_TX_WORDS + 1)) ? -ENOBUFS : 0;
	case 3U:
		words[0] = RC_RAW_PULSE_US(100U);
		words[1] = RC_RAW_SPACE_US(100U);
		return 2;
	default:
		return -EINVAL;
	}
}

RC_PROTOCOL_DEFINE(test_proto_tx_bad) = {
	.name = "test-tx-bad",
	.proto_first = TEST_PROTO_TX_BAD,
	.proto_last = TEST_PROTO_TX_BAD,
	.encode = bad_encode,
};

static int far_encode(uint16_t proto, uint32_t scancode, uint16_t flags, uint32_t *words,
		      size_t max)
{
	ARG_UNUSED(proto);
	ARG_UNUSED(scancode);

	enc_record(false, flags, max);
	words[0] = RC_RAW_PULSE_US(100U);

	return 1;
}

RC_PROTOCOL_DEFINE(test_proto_tx_far) = {
	.name = "test-tx-far",
	.proto_first = TEST_PROTO_TX_FAR,
	.proto_last = TEST_PROTO_TX_FAR,
	.carrier_hz = 70000U,
	.encode = far_encode,
};

static int long_encode(uint16_t proto, uint32_t scancode, uint16_t flags, uint32_t *words,
		       size_t max)
{
	ARG_UNUSED(proto);
	ARG_UNUSED(scancode);

	enc_record(false, flags, max);
	words[0] = RC_RAW_PULSE_US(30000U);
	words[1] = RC_RAW_SPACE_US(20000U);
	words[2] = RC_RAW_PULSE_US(10000U);

	return 3;
}

RC_PROTOCOL_DEFINE(test_proto_tx_long) = {
	.name = "test-tx-long",
	.proto_first = TEST_PROTO_TX_LONG,
	.proto_last = TEST_PROTO_TX_LONG,
	.min_timeout_us = LONG_GAP_MS * 1000U,
	.tx_period_ms = LONG_PERIOD_MS,
	.encode = long_encode,
};

static int rt_encode(uint16_t proto, uint32_t scancode, uint16_t flags, uint32_t *words, size_t max)
{
	ARG_UNUSED(proto);

	enc_record(false, flags, max);
	words[0] = RC_RAW_PULSE_US(1000U + (scancode & 0xfffU) +
				   (((flags & RC_EVENT_FLAG_TOGGLE) != 0U) ? TOG_BIT : 0U));

	return 1;
}

static int rt_encode_repeat(uint16_t proto, uint32_t scancode, uint16_t flags, uint32_t *words,
			    size_t max)
{
	ARG_UNUSED(proto);
	ARG_UNUSED(scancode);

	enc_record(true, flags, max);
	words[0] = RC_RAW_PULSE_US(REPEAT_US);

	return 1;
}

static int rt_decode(void *state, uint32_t word, struct rc_decode_result *res)
{
	uint32_t d = rc_raw_duration_us(word);

	ARG_UNUSED(state);

	if (!rc_raw_is_pulse(word) || (d < 1000U)) {
		return 0;
	}

	res->proto = TEST_PROTO_TX_RT;
	if (d == REPEAT_US) {
		res->flags = RC_EVENT_FLAG_REPEAT;
		res->scancode = 0U;
	} else {
		res->flags = (((d - 1000U) & TOG_BIT) != 0U) ? RC_EVENT_FLAG_TOGGLE : 0U;
		res->scancode = (d - 1000U) & 0xfffU;
	}

	return 1;
}

RC_PROTOCOL_DEFINE(test_proto_tx_rt) = {
	.name = "test-tx-rt",
	.proto_first = TEST_PROTO_TX_RT,
	.proto_last = TEST_PROTO_TX_RT,
	.repeat_period_ms = RT_REPEAT_MS,
	.tx_period_ms = RT_PERIOD_MS,
	.decode = rt_decode,
	.encode = rt_encode,
	.encode_repeat = rt_encode_repeat,
};

static int send_timeout(const struct device *dev, uint16_t proto, uint32_t scancode,
			uint16_t repeats, k_timeout_t timeout)
{
	const struct rc_tx_req req = {
		.proto = proto,
		.repeats = repeats,
		.scancode = scancode,
	};

	return rc_send_sync(dev, &req, timeout);
}

static int send(const struct device *dev, uint16_t proto, uint32_t scancode, uint16_t repeats)
{
	return send_timeout(dev, proto, scancode, repeats, K_MSEC(1000));
}

static void expect_sent(const struct device *dev, const uint32_t *words, size_t count, uint32_t hz,
			uint8_t duty)
{
	uint32_t got[CONFIG_RC_LOOPBACK_TX_WORDS];
	uint32_t got_hz;
	uint8_t got_duty;

	zassert_equal(rc_loopback_tx_get(dev, got, ARRAY_SIZE(got), &got_hz, &got_duty), count);
	zassert_mem_equal(got, words, count * sizeof(words[0]));
	zassert_equal(got_hz, hz, "carrier %u", got_hz);
	zassert_equal(got_duty, duty);
}

static void expect_spacing(size_t count, int64_t min_ms, int64_t max_ms)
{
	zassert_equal(tx_count, count, "%zu transmissions", tx_count);
	for (size_t i = 1; i < count; i++) {
		int64_t us = k_ticks_to_us_floor64(tx_ticks[i] - tx_ticks[i - 1]);

		zassert_true(us + (int64_t)k_ticks_to_us_ceil64(1) >= min_ms * 1000,
			     "frame %zu after %lld us", i, us);
		zassert_true(us < max_ms * 1000, "frame %zu after %lld us", i, us);
	}
}

static void rc_send_before(void *fixture)
{
	ARG_UNUSED(fixture);

	enc_count = 0U;
	tx_count = 0U;
	rc_loopback_tx_cb_set(dev12, tx_record);
	zassert_ok(rc_set_tx_carrier(dev12, DT_PROP(DT_NODELABEL(rc_loopback12), carrier_hz),
				     DT_PROP(DT_NODELABEL(rc_loopback12), duty_cycle_percent)));
}

static void rc_send_after(void *fixture)
{
	ARG_UNUSED(fixture);

	rc_loopback_tx_cb_set(dev12, NULL);
	/* RTIO frees a submission just after its completion */
	k_msleep(5);
	zassert_is_null(rtio_cqe_consume(&send_ctx), "completion left");
	zassert_equal(send_ctx.sqe_pool->pool_free, send_ctx.sqe_pool->pool_size, "request leaked");
}

ZTEST(rc_send, test_send_frame)
{
	const uint32_t words[] = {RC_RAW_PULSE_US(1012), RC_RAW_SPACE_US(500),
				  RC_RAW_PULSE_US(500)};

	zassert_ok(send(dev12, TEST_PROTO_TX_REP, 12U, 0U));
	zassert_equal(tx_count, 1U);
	zassert_equal(enc_count, 1U);
	zassert_false(enc_log[0].repeat);
	zassert_equal(enc_log[0].max, MIN(CONFIG_RC_TX_BUFFER_WORDS, CONFIG_RC_LOOPBACK_TX_WORDS));
	expect_sent(dev12, words, ARRAY_SIZE(words), 36000U, 50U);
}

ZTEST(rc_send, test_repeat_code)
{
	const uint32_t repeat[] = {RC_RAW_PULSE_US(REPEAT_US)};

	zassert_ok(send(dev12, TEST_PROTO_TX_REP, 12U, 2U));
	expect_spacing(3U, REP_PERIOD_MS, REP_PERIOD_MS + SLACK_MS);
	zassert_equal(enc_count, 3U);
	zassert_true(enc_log[0].repeat);
	zassert_false(enc_log[1].repeat);
	zassert_true(enc_log[2].repeat);
	zassert_equal(enc_log[1].flags, enc_log[0].flags);
	zassert_equal(enc_log[2].flags, enc_log[0].flags);
	expect_sent(dev12, repeat, ARRAY_SIZE(repeat), 36000U, 50U);
}

ZTEST(rc_send, test_full_frame_repeats)
{
	uint32_t word;

	zassert_ok(send(dev12, TEST_PROTO_TX_TOG, 5U, 2U));
	expect_spacing(3U, TOG_PERIOD_MS, TOG_PERIOD_MS + SLACK_MS);
	zassert_equal(enc_count, 1U);
	word = RC_RAW_PULSE_US(2005U +
			       (((enc_log[0].flags & RC_EVENT_FLAG_TOGGLE) != 0U) ? TOG_BIT : 0U));
	expect_sent(dev12, &word, 1U, DT_PROP(DT_NODELABEL(rc_loopback12), carrier_hz),
		    DT_PROP(DT_NODELABEL(rc_loopback12), duty_cycle_percent));
}

ZTEST(rc_send, test_frame_longer_than_period)
{
	zassert_ok(send(dev12, TEST_PROTO_TX_LONG, 0U, 2U));
	expect_spacing(3U, LONG_FRAME_MS + LONG_GAP_MS, LONG_FRAME_MS + LONG_GAP_MS + SLACK_MS);
}

ZTEST(rc_send, test_repeats_without_period)
{
	zassert_equal(send(dev12, TEST_PROTO_TX_BAD, 3U, 1U), -EINVAL);
	zassert_equal(enc_count, 0U);
	zassert_equal(tx_count, 0U);
}

ZTEST(rc_send, test_max_duration)
{
	zassert_equal(send(dev12, TEST_PROTO_TX_REP, 1U, 5U), -E2BIG);
	zassert_equal(enc_count, 2U);
	zassert_equal(tx_count, 0U);

	zassert_ok(send(dev12, TEST_PROTO_TX_REP, 1U, 4U));
	zassert_equal(tx_count, 5U);
}

ZTEST(rc_send, test_timeout_while_sending)
{
	for (int i = 0; i < CONFIG_RC_SEND_SYNC_MAX + 1; i++) {
		int64_t start = k_uptime_get();

		tx_count = 0U;
		zassert_equal(send_timeout(dev12, TEST_PROTO_TX_TOG, 1U, 4U, K_MSEC(75)), -EAGAIN);
		zassert_true(k_uptime_get() - start < 150);
		zassert_equal(tx_count, 2U, "%zu transmissions", tx_count);
		k_msleep(2 * TOG_PERIOD_MS);
		zassert_equal(tx_count, 2U, "%zu transmissions", tx_count);
	}

	zassert_ok(send(dev12, TEST_PROTO_TX_TOG, 1U, 0U));
}

ZTEST(rc_send, test_iodev_cancel_while_sending)
{
	static const struct rc_tx_req req = {
		.proto = TEST_PROTO_TX_TOG,
		.repeats = 4U,
		.scancode = 7U,
	};
	struct rtio_sqe *sqe = rtio_sqe_acquire(&send_ctx);
	rtio_sqe_handle_t handle;
	struct rtio_cqe *cqe;

	zassert_not_null(sqe);
	rc_sqe_prep_send(sqe, &send_iodev, &req, NULL);
	handle = rtio_sqe_handle(&send_ctx, sqe);
	zassert_ok(rtio_submit(&send_ctx, 0));
	k_msleep(TOG_PERIOD_MS + (TOG_PERIOD_MS / 2));

	zassert_ok(rtio_sqe_cancel(&send_ctx, handle));
	cqe = rtio_cqe_consume_block(&send_ctx);
	zassert_equal(cqe->result, -ECANCELED);
	rtio_cqe_release(&send_ctx, cqe);
	zassert_equal(tx_count, 2U, "%zu transmissions", tx_count);
	k_msleep(2 * TOG_PERIOD_MS);
	zassert_is_null(rtio_cqe_consume(&send_ctx), "second completion");
	zassert_equal(tx_count, 2U, "%zu transmissions", tx_count);
}

ZTEST(rc_send, test_toggle_per_request)
{
	zassert_ok(send(dev12, TEST_PROTO_TX_TOG, 5U, 1U));
	zassert_ok(send(dev12, TEST_PROTO_TX_TOG, 5U, 1U));
	zassert_ok(send(dev12, TEST_PROTO_TX_TOG, 5U, 0U));
	zassert_equal(enc_count, 3U);
	zassert_not_equal(enc_log[1].flags, enc_log[0].flags);
	zassert_equal(enc_log[2].flags, enc_log[0].flags);
}

ZTEST(rc_send, test_toggle_kept_on_error)
{
	zassert_ok(send(dev12, TEST_PROTO_TX_TOG, 5U, 0U));
	zassert_equal(send(dev12, TEST_PROTO_TX_REP, 0x1000U, 0U), -EINVAL);
	zassert_ok(send(dev12, TEST_PROTO_TX_TOG, 5U, 0U));
	zassert_equal(enc_count, 3U);
	zassert_not_equal(enc_log[2].flags, enc_log[0].flags);
}

ZTEST(rc_send, test_toggle_per_device)
{
	zassert_ok(send(dev12, TEST_PROTO_TX_TOG, 5U, 0U));
	zassert_ok(send(dev10, TEST_PROTO_TX_TOG, 5U, 0U));
	zassert_ok(send(dev10, TEST_PROTO_TX_TOG, 5U, 0U));
	zassert_ok(send(dev12, TEST_PROTO_TX_TOG, 5U, 0U));
	zassert_equal(enc_count, 4U);
	zassert_not_equal(enc_log[3].flags, enc_log[0].flags);
	zassert_not_equal(enc_log[2].flags, enc_log[1].flags);
}

ZTEST(rc_send, test_encoder_errors)
{
	zassert_equal(send(dev12, TEST_PROTO_TX_BAD, 1U, 0U), -EINVAL);
	zassert_equal(send(dev12, TEST_PROTO_TX_BAD, 2U, 0U), -ENOBUFS);
	zassert_equal(send(dev12, TEST_PROTO_TX_BAD, 3U, 0U), -EINVAL);
	zassert_equal(send(dev12, TEST_PROTO_TX_REP, 0x1000U, 0U), -EINVAL);
	zassert_equal(tx_count, 0U);
}

ZTEST(rc_send, test_not_supported)
{
	zassert_equal(send(dev12, TEST_PROTO_NONE, 0U, 0U), -ENOTSUP);
	zassert_equal(send(dev12, TEST_PROTO_KEY_A, 0U, 0U), -ENOTSUP);
	zassert_equal(send(dev11, TEST_PROTO_TX_TOG, 0U, 0U), -ENOTSUP);
	zassert_equal(send(dev12, TEST_PROTO_TX_FAR, 0U, 0U), -ENOTSUP);
	zassert_equal(enc_count, 0U);
	zassert_equal(tx_count, 0U);
}

ZTEST(rc_send, test_fixed_carrier)
{
	const uint32_t word = RC_RAW_PULSE_US(100U);
	uint32_t before = rc_loopback_tx_total(dev14);

	zassert_ok(send(dev14, TEST_PROTO_TX_FAR, 0U, 0U));
	zassert_ok(send(dev14, TEST_PROTO_TX_TOG, 5U, 0U));
	zassert_ok(rc_send_raw_sync(dev14, &word, 1U, K_MSEC(1000)));
	zassert_equal(rc_loopback_tx_total(dev14), before + 3U);
	expect_sent(dev14, &word, 1U, 0U, 0U);
}

ZTEST(rc_send, test_raw_carrier_restored)
{
	const uint32_t word = RC_RAW_PULSE_US(300);

	zassert_ok(rc_set_tx_carrier(dev12, 40000U, 25U));
	zassert_ok(send(dev12, TEST_PROTO_TX_REP, 1U, 0U));
	zassert_ok(rc_send_raw_sync(dev12, &word, 1U, K_MSEC(1000)));
	expect_sent(dev12, &word, 1U, 40000U, 25U);
}

static void expect_event(uint32_t scancode, uint16_t flags)
{
	struct rtio_cqe *cqe = NULL;
	struct rc_event evt;
	uint32_t len = 0U;
	uint8_t *buf = NULL;

	for (int i = 0; (i < 2 * (RT_REPEAT_MS + RT_PERIOD_MS)) && (cqe == NULL); i++) {
		cqe = rtio_cqe_consume(&rt_ctx);
		if (cqe == NULL) {
			k_sleep(K_MSEC(1));
		}
	}

	zassert_not_null(cqe, "no event");
	zassert_equal(cqe->result, 0, "result %d", cqe->result);
	zassert_ok(rtio_cqe_get_mempool_buffer(&rt_ctx, cqe, &buf, &len));
	zassert_true(len >= sizeof(evt));
	memcpy(&evt, buf, sizeof(evt));
	rtio_release_buffer(&rt_ctx, buf, len);
	rtio_cqe_release(&rt_ctx, cqe);

	zassert_equal(evt.proto, TEST_PROTO_TX_RT);
	zassert_equal(evt.scancode, scancode, "scancode 0x%x", evt.scancode);
	zassert_equal(evt.flags, flags, "flags 0x%x", evt.flags);
}

ZTEST(rc_send, test_round_trip)
{
	rtio_sqe_handle_t stream = 0;
	struct rtio_cqe *cqe;
	uint16_t tog;
	int result;

	zassert_ok(rc_rx_enable(dev13, true));
	zassert_ok(rc_stream(&rt_iodev, &rt_ctx, NULL, &stream));

	zassert_ok(send(dev13, TEST_PROTO_TX_RT, 0x42U, 2U));
	tog = enc_log[1].flags & RC_EVENT_FLAG_TOGGLE;
	expect_event(0x42U, tog);
	expect_event(0x42U, tog | RC_EVENT_FLAG_REPEAT);
	expect_event(0x42U, tog | RC_EVENT_FLAG_REPEAT);
	expect_event(0x42U, tog | RC_EVENT_FLAG_RELEASE);

	zassert_ok(send(dev13, TEST_PROTO_TX_RT, 0x42U, 0U));
	expect_event(0x42U, tog ^ RC_EVENT_FLAG_TOGGLE);
	expect_event(0x42U, (tog ^ RC_EVENT_FLAG_TOGGLE) | RC_EVENT_FLAG_RELEASE);

	zassert_ok(rc_rx_enable(dev13, false));
	zassert_ok(rtio_sqe_cancel(&rt_ctx, stream));
	do {
		uint32_t len = 0U;
		uint8_t *buf = NULL;

		cqe = rtio_cqe_consume_block(&rt_ctx);
		result = cqe->result;
		if ((rtio_cqe_get_mempool_buffer(&rt_ctx, cqe, &buf, &len) == 0) && (buf != NULL)) {
			rtio_release_buffer(&rt_ctx, buf, len);
		}
		rtio_cqe_release(&rt_ctx, cqe);
	} while (result != -ECANCELED);
}

static int submit_wait(void)
{
	struct rtio_cqe *cqe;
	int result;

	zassert_ok(rtio_submit(&send_ctx, 1));
	cqe = rtio_cqe_consume_block(&send_ctx);
	result = cqe->result;
	rtio_cqe_release(&send_ctx, cqe);

	return result;
}

ZTEST(rc_send, test_iodev)
{
	static const struct rc_tx_req req = {
		.proto = TEST_PROTO_TX_TOG,
		.repeats = 1U,
		.scancode = 7U,
	};
	struct rtio_sqe *sqe;

	sqe = rtio_sqe_acquire(&send_ctx);
	zassert_not_null(sqe);
	rc_sqe_prep_send(sqe, &send_iodev, &req, NULL);
	zassert_ok(submit_wait());
	zassert_equal(tx_count, 2U);
	zassert_equal(enc_count, 1U);

	sqe = rtio_sqe_acquire(&send_ctx);
	zassert_not_null(sqe);
	rtio_sqe_prep_write(sqe, &send_iodev, RTIO_PRIO_NORM, (const uint8_t *)&req,
			    sizeof(req) - 1U, NULL);
	sqe->iodev_flags = RC_IODEV_TX_SCANCODE;
	zassert_equal(submit_wait(), -EINVAL);
	zassert_equal(tx_count, 2U);
}

ZTEST_SUITE(rc_send, NULL, NULL, rc_send_before, rc_send_after, NULL);
