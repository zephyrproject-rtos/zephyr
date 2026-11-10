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
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/ztest.h>

#define P(us)     RC_RAW_PULSE_US(us)
#define S(us)     RC_RAW_SPACE_US(us)
#define EVT_WAIT  K_MSEC(400)
#define QUIET_MS  200
#define MAX_WORDS 48

static const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(rc_rc5_0));

/* Built from the bit strings by hand: S1 S2 T, system, command */
static const uint32_t rc5_1927[] = {
	P(889), S(889),  P(1778), S(1778), P(889), S(889),  P(1778),
	S(889), P(889),  S(1778), P(889),  S(889), P(1778), S(889),
	P(889), S(1778), P(889),  S(889),  P(889), S(889),  P(889),
};
static const uint32_t rc5_0a0d_t[] = {
	P(889), S(889), P(889), S(889),  P(1778), S(1778), P(1778), S(1778), P(1778), S(889),
	P(889), S(889), P(889), S(1778), P(889),  S(889),  P(1778), S(1778), P(889),
};
static const uint32_t rc5x_0a4d15[] = {
	P(1778), S(889),  P(889),  S(889),  P(889),  S(1778), P(1778), S(1778), P(1778),
	S(4445), P(889),  S(889),  P(889),  S(1778), P(889),  S(889),  P(1778), S(1778),
	P(1778), S(1778), P(1778), S(1778), P(1778), S(1778), P(889),
};

K_MSGQ_DEFINE(evt_q, sizeof(struct rc_event), 16, 4);

static void evt_cb(const struct device *d, const struct rc_event *evt, void *user_data)
{
	ARG_UNUSED(d);
	ARG_UNUSED(user_data);

	(void)k_msgq_put(&evt_q, evt, K_NO_WAIT);
}

RC_CALLBACK_DEFINE(DEVICE_DT_GET(DT_NODELABEL(rc_rc5_0)), evt_cb, NULL);

static const struct rc_protocol *rc5_protocol(void)
{
	STRUCT_SECTION_FOREACH(rc_protocol, p) {
		if (strcmp(p->name, "rc5") == 0) {
			return p;
		}
	}

	return NULL;
}

static void expect_event(uint16_t proto, uint32_t scancode, uint16_t flags)
{
	struct rc_event evt;

	zassert_ok(k_msgq_get(&evt_q, &evt, EVT_WAIT), "no event");
	zassert_equal(evt.proto, proto, "proto %u", evt.proto);
	zassert_equal(evt.scancode, scancode, "scancode 0x%x", evt.scancode);
	zassert_equal(evt.flags, flags, "flags 0x%x", evt.flags);
}

static void expect_none(void)
{
	struct rc_event evt;

	zassert_equal(k_msgq_get(&evt_q, &evt, K_MSEC(QUIET_MS)), -EAGAIN, "unexpected event");
}

static void expect_key(const uint32_t *w, size_t count, uint16_t proto, uint32_t scancode,
		       uint16_t toggle)
{
	zassert_ok(rc_loopback_inject(dev, w, count));
	expect_event(proto, scancode, toggle);
	expect_event(proto, scancode, toggle | RC_EVENT_FLAG_RELEASE);
}

static void inject_changed(const uint32_t *w, size_t count, size_t i, uint32_t us)
{
	uint32_t copy[MAX_WORDS];

	memcpy(copy, w, count * sizeof(w[0]));
	copy[i] = rc_raw_is_pulse(w[i]) ? P(us) : S(us);
	zassert_ok(rc_loopback_inject(dev, copy, count));
}

static uint16_t send_key(uint16_t proto, uint32_t scancode)
{
	const struct rc_tx_req req = {.proto = proto, .scancode = scancode};
	struct rc_event evt;

	zassert_ok(rc_send_sync(dev, &req, K_SECONDS(1)));
	zassert_ok(k_msgq_get(&evt_q, &evt, EVT_WAIT), "no event");
	zassert_equal(evt.proto, proto);
	zassert_equal(evt.scancode, scancode, "scancode 0x%x", evt.scancode);
	expect_event(proto, scancode, evt.flags | RC_EVENT_FLAG_RELEASE);

	return evt.flags & RC_EVENT_FLAG_TOGGLE;
}

static void rc5_before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_msleep(QUIET_MS);
	k_msgq_purge(&evt_q);
	zassert_ok(rc_rx_enable(dev, true));
}

static void rc5_after(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(rc_rx_enable(dev, false));
}

ZTEST(rc_rc5, test_round_trip)
{
	static const struct {
		uint16_t proto;
		uint32_t scancode;
	} keys[] = {
		{RC_PROTO_RC5, 0x1927U},       {RC_PROTO_RC5, 0x0a0dU},
		{RC_PROTO_RC5, 0x1f7fU},       {RC_PROTO_RC5, 0x0000U},
		{RC_PROTO_RC5X_20, 0x0a4d15U}, {RC_PROTO_RC5X_20, 0x1f7f3fU},
		{RC_PROTO_RC5X_20, 0x0U},
	};

	ARRAY_FOR_EACH(keys, i) {
		(void)send_key(keys[i].proto, keys[i].scancode);
	}
}

ZTEST(rc_rc5, test_toggle)
{
	uint16_t first = send_key(RC_PROTO_RC5, 0x0102U);
	uint16_t second = send_key(RC_PROTO_RC5, 0x0102U);

	zassert_not_equal(first, second, "toggle not flipped");
	first = send_key(RC_PROTO_RC5X_20, 0x010203U);
	second = send_key(RC_PROTO_RC5X_20, 0x010203U);
	zassert_not_equal(first, second, "toggle not flipped");
}

ZTEST(rc_rc5, test_decode_vectors)
{
	expect_key(rc5_1927, ARRAY_SIZE(rc5_1927), RC_PROTO_RC5, 0x1927U, 0U);
	expect_key(rc5_0a0d_t, ARRAY_SIZE(rc5_0a0d_t), RC_PROTO_RC5, 0x0a0dU, RC_EVENT_FLAG_TOGGLE);
	expect_key(rc5x_0a4d15, ARRAY_SIZE(rc5x_0a4d15), RC_PROTO_RC5X_20, 0x0a4d15U, 0U);
}

ZTEST(rc_rc5, test_encode)
{
	const struct rc_protocol *p = rc5_protocol();
	uint32_t got[MAX_WORDS];

	zassert_not_null(p);
	zassert_equal(p->encode(RC_PROTO_RC5, 0x1927U, 0U, got, ARRAY_SIZE(got)),
		      ARRAY_SIZE(rc5_1927));
	zassert_mem_equal(got, rc5_1927, sizeof(rc5_1927));
	zassert_equal(p->encode(RC_PROTO_RC5, 0x0a0dU, RC_EVENT_FLAG_TOGGLE, got, ARRAY_SIZE(got)),
		      ARRAY_SIZE(rc5_0a0d_t));
	zassert_mem_equal(got, rc5_0a0d_t, sizeof(rc5_0a0d_t));
	zassert_equal(p->encode(RC_PROTO_RC5X_20, 0x0a4d15U, 0U, got, ARRAY_SIZE(got)),
		      ARRAY_SIZE(rc5x_0a4d15));
	zassert_mem_equal(got, rc5x_0a4d15, sizeof(rc5x_0a4d15));

	zassert_equal(p->encode(RC_PROTO_RC5, 0x2000U, 0U, got, ARRAY_SIZE(got)), -EINVAL);
	zassert_equal(p->encode(RC_PROTO_RC5, 0x0080U, 0U, got, ARRAY_SIZE(got)), -EINVAL);
	zassert_equal(p->encode(RC_PROTO_RC5X_20, 0x000040U, 0U, got, ARRAY_SIZE(got)), -EINVAL);
	zassert_equal(p->encode(RC_PROTO_NEC, 0x1U, 0U, got, ARRAY_SIZE(got)), -EINVAL);
	zassert_equal(p->encode(RC_PROTO_RC5, 0x1927U, 0U, got, ARRAY_SIZE(rc5_1927) - 1U),
		      -ENOBUFS);
	zassert_is_null(p->encode_repeat);
}

ZTEST(rc_rc5, test_half_bit_boundary)
{
	/* 1 and 2 half bits split at 1333.5 us */
	inject_changed(rc5_1927, ARRAY_SIZE(rc5_1927), 1, 1333);
	expect_event(RC_PROTO_RC5, 0x1927U, 0U);
	expect_event(RC_PROTO_RC5, 0x1927U, RC_EVENT_FLAG_RELEASE);
	inject_changed(rc5_1927, ARRAY_SIZE(rc5_1927), 1, 1334);
	expect_none();

	inject_changed(rc5_1927, ARRAY_SIZE(rc5_1927), 2, 1334);
	expect_event(RC_PROTO_RC5, 0x1927U, 0U);
	expect_event(RC_PROTO_RC5, 0x1927U, RC_EVENT_FLAG_RELEASE);
	inject_changed(rc5_1927, ARRAY_SIZE(rc5_1927), 2, 1333);
	expect_none();

	/* Shortest and longest accepted single half bit */
	inject_changed(rc5_1927, ARRAY_SIZE(rc5_1927), 0, 445);
	expect_event(RC_PROTO_RC5, 0x1927U, 0U);
	expect_event(RC_PROTO_RC5, 0x1927U, RC_EVENT_FLAG_RELEASE);
	inject_changed(rc5_1927, ARRAY_SIZE(rc5_1927), 0, 444);
	expect_none();
}

ZTEST(rc_rc5, test_gap_boundary)
{
	/* The RC5X gap here is one half bit plus 4 units: 5 units split at 4000.5 and 4889.5 us */
	inject_changed(rc5x_0a4d15, ARRAY_SIZE(rc5x_0a4d15), 9, 4001);
	expect_event(RC_PROTO_RC5X_20, 0x0a4d15U, 0U);
	expect_event(RC_PROTO_RC5X_20, 0x0a4d15U, RC_EVENT_FLAG_RELEASE);
	inject_changed(rc5x_0a4d15, ARRAY_SIZE(rc5x_0a4d15), 9, 4000);
	expect_none();

	inject_changed(rc5x_0a4d15, ARRAY_SIZE(rc5x_0a4d15), 9, 4889);
	expect_event(RC_PROTO_RC5X_20, 0x0a4d15U, 0U);
	expect_event(RC_PROTO_RC5X_20, 0x0a4d15U, RC_EVENT_FLAG_RELEASE);
	inject_changed(rc5x_0a4d15, ARRAY_SIZE(rc5x_0a4d15), 9, 4890);
	expect_none();
}

ZTEST(rc_rc5, test_trailer)
{
	/* A frame is reported once a 4890 us space follows its last bit, else it is dropped */
	static const struct {
		uint32_t scancode;
		uint32_t space;
		bool ok;
	} cases[] = {
		{0x1927U, 4890U, true},
		{0x1927U, 4889U, false},
		{0x1926U, 5779U, true},
		{0x1926U, 5778U, false},
	};
	const struct rc_protocol *p = rc5_protocol();
	uint32_t w[MAX_WORDS];

	ARRAY_FOR_EACH(cases, i) {
		int n = p->encode(RC_PROTO_RC5, cases[i].scancode, 0U, w, ARRAY_SIZE(w) - 2U);

		zassert_true(n > 0);
		w[n] = S(cases[i].space);
		w[n + 1] = P(889U);
		zassert_ok(rc_loopback_inject(dev, w, n + 2));
		if (cases[i].ok) {
			expect_event(RC_PROTO_RC5, cases[i].scancode, 0U);
			expect_event(RC_PROTO_RC5, cases[i].scancode, RC_EVENT_FLAG_RELEASE);
		} else {
			expect_none();
		}
	}
}

ZTEST(rc_rc5, test_truncated)
{
	zassert_ok(rc_loopback_inject(dev, rc5_1927, 10));
	expect_none();
	expect_key(rc5_0a0d_t, ARRAY_SIZE(rc5_0a0d_t), RC_PROTO_RC5, 0x0a0dU, RC_EVENT_FLAG_TOGGLE);
}

ZTEST(rc_rc5, test_long_mark_rejected)
{
	inject_changed(rc5_1927, ARRAY_SIZE(rc5_1927), 2, 2667);
	expect_none();
	expect_key(rc5_1927, ARRAY_SIZE(rc5_1927), RC_PROTO_RC5, 0x1927U, 0U);
}

ZTEST(rc_rc5, test_restart)
{
	uint32_t w[MAX_WORDS];

	/* After a broken frame the next one starts only after a trailer space */
	memcpy(w, rc5_1927, 9 * sizeof(w[0]));
	w[9] = S(4890U);
	memcpy(&w[10], rc5_0a0d_t, sizeof(rc5_0a0d_t));
	zassert_ok(rc_loopback_inject(dev, w, 10 + ARRAY_SIZE(rc5_0a0d_t)));
	expect_event(RC_PROTO_RC5, 0x0a0dU, RC_EVENT_FLAG_TOGGLE);
	expect_event(RC_PROTO_RC5, 0x0a0dU, RC_EVENT_FLAG_TOGGLE | RC_EVENT_FLAG_RELEASE);

	w[9] = S(4889U);
	zassert_ok(rc_loopback_inject(dev, w, 10 + ARRAY_SIZE(rc5_0a0d_t)));
	expect_none();
}

ZTEST_SUITE(rc_rc5, NULL, NULL, rc5_before, rc5_after, NULL);
