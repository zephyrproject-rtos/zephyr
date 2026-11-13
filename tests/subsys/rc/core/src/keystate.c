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

#define TEST_PROTO_KEY_A 0x1fb
#define TEST_PROTO_KEY_B 0x1fc
#define TEST_PROTO_OTHER 0x1ff

#define KEY_SCANCODE_MASK 0xfffU
#define KEY_TOGGLE        BIT(12)
#define KEY_REPEAT_CODE   BIT(13)
#define KEY_PROTO_B       BIT(14)
#define KEY_OTHER         BIT(15)

#define REPEAT_PERIOD_MS 50
#define RX_TIMEOUT_MS    (DT_PROP(DT_NODELABEL(rc_loopback6), rx_timeout_us) / 1000)
#define KEYUP_MS         (RX_TIMEOUT_MS + REPEAT_PERIOD_MS)

static int test_key_decode(void *state, uint32_t word, struct rc_decode_result *res)
{
	uint32_t d = rc_raw_duration_us(word);

	ARG_UNUSED(state);

	if (!rc_raw_is_pulse(word) || ((d & KEY_OTHER) != 0U)) {
		return 0;
	}

	res->proto = ((d & KEY_PROTO_B) != 0U) ? TEST_PROTO_KEY_B : TEST_PROTO_KEY_A;
	res->scancode = d & KEY_SCANCODE_MASK;
	res->flags = (((d & KEY_TOGGLE) != 0U) ? RC_EVENT_FLAG_TOGGLE : 0U) |
		     (((d & KEY_REPEAT_CODE) != 0U) ? RC_EVENT_FLAG_REPEAT : 0U);

	return 1;
}

RC_PROTOCOL_DEFINE(test_proto_key) = {
	.name = "test-key",
	.proto_first = TEST_PROTO_KEY_A,
	.proto_last = TEST_PROTO_KEY_B,
	.repeat_period_ms = REPEAT_PERIOD_MS,
	.decode = test_key_decode,
};

static int test_other_decode(void *state, uint32_t word, struct rc_decode_result *res)
{
	ARG_UNUSED(state);

	if (!rc_raw_is_pulse(word) || ((rc_raw_duration_us(word) & KEY_OTHER) == 0U)) {
		return 0;
	}

	res->proto = TEST_PROTO_OTHER;
	res->scancode = 0U;
	res->flags = RC_EVENT_FLAG_REPEAT;

	return 1;
}

RC_PROTOCOL_DEFINE(test_proto_other) = {
	.name = "test-other",
	.proto_first = TEST_PROTO_OTHER,
	.proto_last = TEST_PROTO_OTHER,
	.repeat_period_ms = REPEAT_PERIOD_MS,
	.decode = test_other_decode,
};

RC_DT_EVENT_IODEV(key_iodev, DT_NODELABEL(rc_loopback6));
RTIO_DEFINE_WITH_MEMPOOL(key_ctx, 4, 16, 16, 32, sizeof(void *));

static const struct device *const dev6 = DEVICE_DT_GET(DT_NODELABEL(rc_loopback6));
static rtio_sqe_handle_t stream;

static void frame(uint32_t d)
{
	const uint32_t word = RC_RAW_PULSE_US(d);

	zassert_ok(rc_loopback_inject(dev6, &word, 1));
}

static bool next_event(struct rc_event *evt, int timeout_ms)
{
	for (int i = 0; i <= timeout_ms; i++) {
		struct rtio_cqe *cqe = rtio_cqe_consume(&key_ctx);
		uint32_t len = 0U;
		uint8_t *buf = NULL;

		if (cqe == NULL) {
			k_sleep(K_MSEC(1));
			continue;
		}

		zassert_equal(cqe->result, 0, "result %d", cqe->result);
		zassert_ok(rtio_cqe_get_mempool_buffer(&key_ctx, cqe, &buf, &len));
		zassert_true(len >= sizeof(*evt));
		memcpy(evt, buf, sizeof(*evt));
		rtio_release_buffer(&key_ctx, buf, len);
		rtio_cqe_release(&key_ctx, cqe);

		return true;
	}

	return false;
}

static struct rc_event expect(uint16_t proto, uint32_t scancode, uint16_t flags)
{
	struct rc_event evt;

	zassert_true(next_event(&evt, 2 * KEYUP_MS), "no event");
	zassert_equal(evt.proto, proto, "proto 0x%x", evt.proto);
	zassert_equal(evt.scancode, scancode, "scancode 0x%x", evt.scancode);
	zassert_equal(evt.flags, flags, "flags 0x%x", evt.flags);

	return evt;
}

static void expect_none(int timeout_ms)
{
	struct rc_event evt = {0};

	zassert_false(next_event(&evt, timeout_ms), "unexpected event flags 0x%x scancode 0x%x",
		      evt.flags, evt.scancode);
}

static void rc_key_before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_sleep(K_MSEC(2 * KEYUP_MS));
	zassert_ok(rc_rx_enable(dev6, true));
	zassert_ok(rc_stream(&key_iodev, &key_ctx, NULL, &stream));
}

static void rc_key_after(void *fixture)
{
	struct rtio_cqe *cqe;
	int result;

	ARG_UNUSED(fixture);

	zassert_ok(rc_rx_enable(dev6, false));
	zassert_ok(rtio_sqe_cancel(&key_ctx, stream));

	do {
		uint32_t len = 0U;
		uint8_t *buf = NULL;

		cqe = rtio_cqe_consume_block(&key_ctx);
		result = cqe->result;
		if ((rtio_cqe_get_mempool_buffer(&key_ctx, cqe, &buf, &len) == 0) &&
		    (buf != NULL)) {
			rtio_release_buffer(&key_ctx, buf, len);
		}
		rtio_cqe_release(&key_ctx, cqe);
	} while (result != -ECANCELED);
}

ZTEST(rc_key, test_new_key)
{
	frame(0x10);
	expect(TEST_PROTO_KEY_A, 0x10, 0);
}

ZTEST(rc_key, test_held_repeat)
{
	frame(0x10);
	frame(0x10);
	expect(TEST_PROTO_KEY_A, 0x10, 0);
	expect(TEST_PROTO_KEY_A, 0x10, RC_EVENT_FLAG_REPEAT);
}

ZTEST(rc_key, test_toggle_new_press)
{
	frame(0x10);
	frame(0x10 | KEY_TOGGLE);
	frame(0x10 | KEY_TOGGLE);
	expect(TEST_PROTO_KEY_A, 0x10, 0);
	expect(TEST_PROTO_KEY_A, 0x10, RC_EVENT_FLAG_RELEASE);
	expect(TEST_PROTO_KEY_A, 0x10, RC_EVENT_FLAG_TOGGLE);
	expect(TEST_PROTO_KEY_A, 0x10, RC_EVENT_FLAG_TOGGLE | RC_EVENT_FLAG_REPEAT);
}

ZTEST(rc_key, test_repeat_code)
{
	frame(0x10 | KEY_TOGGLE);
	frame(KEY_REPEAT_CODE);
	expect(TEST_PROTO_KEY_A, 0x10, RC_EVENT_FLAG_TOGGLE);
	expect(TEST_PROTO_KEY_A, 0x10, RC_EVENT_FLAG_TOGGLE | RC_EVENT_FLAG_REPEAT);
}

ZTEST(rc_key, test_repeat_code_idle)
{
	frame(KEY_REPEAT_CODE);
	expect_none(20);
}

ZTEST(rc_key, test_release_after_timeout)
{
	int64_t start = k_uptime_get();
	struct rc_event pressed, released;

	frame(0x10 | KEY_TOGGLE);
	pressed = expect(TEST_PROTO_KEY_A, 0x10, RC_EVENT_FLAG_TOGGLE);
	released = expect(TEST_PROTO_KEY_A, 0x10, RC_EVENT_FLAG_TOGGLE | RC_EVENT_FLAG_RELEASE);

	zassert_true(k_uptime_get() - start >= KEYUP_MS, "released after %lld ms",
		     k_uptime_get() - start);
	zassert_true(released.timestamp_ns - pressed.timestamp_ns >= KEYUP_MS * NSEC_PER_MSEC);
	expect_none(2 * KEYUP_MS);
}

ZTEST(rc_key, test_keyup_extended)
{
	struct rc_event last, released;

	frame(0x10);
	last = expect(TEST_PROTO_KEY_A, 0x10, 0);
	for (int i = 0; i < 3; i++) {
		k_sleep(K_MSEC(KEYUP_MS / 2));
		frame((i == 1) ? KEY_REPEAT_CODE : 0x10);
		last = expect(TEST_PROTO_KEY_A, 0x10, RC_EVENT_FLAG_REPEAT);
	}

	released = expect(TEST_PROTO_KEY_A, 0x10, RC_EVENT_FLAG_RELEASE);
	zassert_true(released.timestamp_ns - last.timestamp_ns >= KEYUP_MS * NSEC_PER_MSEC,
		     "released %llu ns after the last frame",
		     released.timestamp_ns - last.timestamp_ns);
}

ZTEST(rc_key, test_other_scancode)
{
	frame(0x10);
	frame(0x11);
	expect(TEST_PROTO_KEY_A, 0x10, 0);
	expect(TEST_PROTO_KEY_A, 0x10, RC_EVENT_FLAG_RELEASE);
	expect(TEST_PROTO_KEY_A, 0x11, 0);
}

ZTEST(rc_key, test_other_proto)
{
	struct rc_event released, pressed;

	frame(0x10);
	frame(0x10 | KEY_PROTO_B);
	expect(TEST_PROTO_KEY_A, 0x10, 0);
	released = expect(TEST_PROTO_KEY_A, 0x10, RC_EVENT_FLAG_RELEASE);
	pressed = expect(TEST_PROTO_KEY_B, 0x10, 0);
	zassert_true(pressed.timestamp_ns >= released.timestamp_ns);

	expect(TEST_PROTO_KEY_B, 0x10, RC_EVENT_FLAG_RELEASE);
}

ZTEST(rc_key, test_repeat_other_protocol)
{
	struct rc_event press;
	struct rc_event release;
	int64_t wait_ms;

	frame(5);
	press = expect(TEST_PROTO_KEY_A, 5, 0);
	wait_ms = (int64_t)(press.timestamp_ns / NSEC_PER_MSEC) + (KEYUP_MS / 2) - k_uptime_get();
	if (wait_ms > 0) {
		k_sleep(K_MSEC(wait_ms));
	}
	frame(KEY_OTHER);
	release = expect(TEST_PROTO_KEY_A, 5, RC_EVENT_FLAG_RELEASE);
	zassert_true(release.timestamp_ns - press.timestamp_ns <
			     (uint64_t)(KEYUP_MS + (KEYUP_MS / 4)) * NSEC_PER_MSEC,
		     "keyup deadline moved");
}

ZTEST_SUITE(rc_key, NULL, NULL, rc_key_before, rc_key_after, NULL);
