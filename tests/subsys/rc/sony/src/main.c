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
#define MAX_WORDS 128

static const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(rc_sony0));

/* Built from the bit strings by hand: header, function, device */
static const uint32_t sony12_010015[] = {
	P(2400), S(600),  P(1200), S(600), P(600), S(600), P(1200), S(600),  P(600),
	S(600),  P(1200), S(600),  P(600), S(600), P(600), S(600),  P(1200), S(600),
	P(600),  S(600),  P(600),  S(600), P(600), S(600), P(600),
};
static const uint32_t sony20_1aa57f[] = {
	P(2400), S(600),  P(1200), S(600),  P(1200), S(600),  P(1200), S(600), P(1200),
	S(600),  P(1200), S(600),  P(1200), S(600),  P(1200), S(600),  P(600), S(600),
	P(1200), S(600),  P(600),  S(600),  P(1200), S(600),  P(1200), S(600), P(1200),
	S(600),  P(600),  S(600),  P(1200), S(600),  P(600),  S(600),  P(600), S(600),
	P(1200), S(600),  P(600),  S(600),  P(1200),
};

static int64_t tx_ticks[4];
static size_t tx_n;

K_MSGQ_DEFINE(evt_q, sizeof(struct rc_event), 16, 4);

static void evt_cb(const struct device *d, const struct rc_event *evt, void *user_data)
{
	ARG_UNUSED(d);
	ARG_UNUSED(user_data);

	(void)k_msgq_put(&evt_q, evt, K_NO_WAIT);
}

RC_CALLBACK_DEFINE(DEVICE_DT_GET(DT_NODELABEL(rc_sony0)), evt_cb, NULL);

static void tx_cb(const struct device *d)
{
	ARG_UNUSED(d);

	if (tx_n < ARRAY_SIZE(tx_ticks)) {
		tx_ticks[tx_n++] = k_uptime_ticks();
	}
}

static const struct rc_protocol *encoder_for(uint16_t proto)
{
	STRUCT_SECTION_FOREACH(rc_protocol, p) {
		if ((p->encode != NULL) && (proto >= p->proto_first) && (proto <= p->proto_last)) {
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

static bool is_sony(uint16_t proto)
{
	return (proto >= RC_PROTO_SONY12) && (proto <= RC_PROTO_SONY20);
}

/* RC5 may legally decode a frame that Sony rejects */
static void expect_no_sony(void)
{
	struct rc_event evt;

	while (k_msgq_get(&evt_q, &evt, K_MSEC(QUIET_MS)) == 0) {
		zassert_false(is_sony(evt.proto), "Sony event proto %u", evt.proto);
	}
}

/* Sony events of a frame that another decoder may also take */
static void expect_sony_key(uint32_t scancode)
{
	static const uint16_t flags[] = {0U, RC_EVENT_FLAG_RELEASE};
	struct rc_event evt;

	ARRAY_FOR_EACH(flags, i) {
		do {
			zassert_ok(k_msgq_get(&evt_q, &evt, EVT_WAIT), "no event");
		} while (!is_sony(evt.proto));
		zassert_equal(evt.proto, RC_PROTO_SONY12, "proto %u", evt.proto);
		zassert_equal(evt.scancode, scancode, "scancode 0x%x", evt.scancode);
		zassert_equal(evt.flags, flags[i], "flags 0x%x", evt.flags);
	}
	expect_no_sony();
}

static void expect_key(uint16_t proto, uint32_t scancode)
{
	expect_event(proto, scancode, 0U);
	expect_event(proto, scancode, RC_EVENT_FLAG_RELEASE);
}

static void inject(const uint32_t *w, size_t count)
{
	zassert_ok(rc_loopback_inject(dev, w, count));
}

static void inject_changed(const uint32_t *w, size_t count, size_t i, uint32_t us)
{
	uint32_t copy[MAX_WORDS];

	memcpy(copy, w, count * sizeof(w[0]));
	copy[i] = rc_raw_is_pulse(w[i]) ? P(us) : S(us);
	inject(copy, count);
}

static void sony_before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_msleep(QUIET_MS);
	k_msgq_purge(&evt_q);
	zassert_ok(rc_rx_enable(dev, true));
}

static void sony_after(void *fixture)
{
	ARG_UNUSED(fixture);

	rc_loopback_tx_cb_set(dev, NULL);
	zassert_ok(rc_rx_enable(dev, false));
}

ZTEST(rc_sony, test_round_trip)
{
	static const struct {
		uint16_t proto;
		uint32_t scancode;
	} keys[] = {
		{RC_PROTO_SONY12, 0x010015U}, {RC_PROTO_SONY12, 0x1f007fU},
		{RC_PROTO_SONY12, 0x000000U}, {RC_PROTO_SONY15, 0xa50042U},
		{RC_PROTO_SONY15, 0xff007fU}, {RC_PROTO_SONY20, 0x1aa57fU},
		{RC_PROTO_SONY20, 0x1fff7fU}, {RC_PROTO_SONY20, 0x000100U},
	};

	ARRAY_FOR_EACH(keys, i) {
		const struct rc_tx_req req = {.proto = keys[i].proto, .scancode = keys[i].scancode};

		zassert_ok(rc_send_sync(dev, &req, K_SECONDS(1)));
		expect_key(keys[i].proto, keys[i].scancode);
	}
}

ZTEST(rc_sony, test_decode_vectors)
{
	inject(sony12_010015, ARRAY_SIZE(sony12_010015));
	expect_key(RC_PROTO_SONY12, 0x010015U);
	inject(sony20_1aa57f, ARRAY_SIZE(sony20_1aa57f));
	expect_key(RC_PROTO_SONY20, 0x1aa57fU);
}

ZTEST(rc_sony, test_encode)
{
	const struct rc_protocol *p = encoder_for(RC_PROTO_SONY12);
	uint32_t got[MAX_WORDS];

	zassert_not_null(p);
	zassert_equal(p->encode(RC_PROTO_SONY12, 0x010015U, 0U, got, ARRAY_SIZE(got)),
		      ARRAY_SIZE(sony12_010015));
	zassert_mem_equal(got, sony12_010015, sizeof(sony12_010015));
	zassert_equal(
		p->encode(RC_PROTO_SONY20, 0x1aa57fU, RC_EVENT_FLAG_TOGGLE, got, ARRAY_SIZE(got)),
		ARRAY_SIZE(sony20_1aa57f));
	zassert_mem_equal(got, sony20_1aa57f, sizeof(sony20_1aa57f));

	zassert_equal(p->encode(RC_PROTO_SONY12, 0x200000U, 0U, got, ARRAY_SIZE(got)), -EINVAL);
	zassert_equal(p->encode(RC_PROTO_SONY12, 0x000100U, 0U, got, ARRAY_SIZE(got)), -EINVAL);
	zassert_equal(p->encode(RC_PROTO_SONY12, 0x000080U, 0U, got, ARRAY_SIZE(got)), -EINVAL);
	zassert_equal(p->encode(RC_PROTO_SONY15, 0x000100U, 0U, got, ARRAY_SIZE(got)), -EINVAL);
	zassert_equal(p->encode(RC_PROTO_SONY20, 0x200000U, 0U, got, ARRAY_SIZE(got)), -EINVAL);
	zassert_equal(p->encode(RC_PROTO_NEC, 0x1U, 0U, got, ARRAY_SIZE(got)), -EINVAL);
	zassert_equal(
		p->encode(RC_PROTO_SONY12, 0x010015U, 0U, got, ARRAY_SIZE(sony12_010015) - 1U),
		-ENOBUFS);
	zassert_is_null(p->encode_repeat);
}

ZTEST(rc_sony, test_boundaries)
{
	/* Mark 0/1 split at 900, bit space from 300 to 900 */
	static const struct {
		size_t word;
		uint32_t us;
		uint32_t scancode;
	} cases[] = {
		{4, 899, 0x010015U}, {4, 900, 0U},         {4, 901, 0x010017U}, {2, 901, 0x010015U},
		{2, 899, 0x010014U}, {2, 1499, 0x010015U}, {2, 1500, 0U},       {4, 301, 0x010015U},
		{4, 300, 0U},        {5, 899, 0x010015U},  {5, 901, 0U},        {5, 301, 0x010015U},
		{5, 300, 0U},
	};

	ARRAY_FOR_EACH(cases, i) {
		inject_changed(sony12_010015, ARRAY_SIZE(sony12_010015), cases[i].word,
			       cases[i].us);
		if (cases[i].scancode != 0U) {
			expect_sony_key(cases[i].scancode);
		} else {
			expect_no_sony();
		}
	}
}

ZTEST(rc_sony, test_header_period)
{
	/* Header space from 300 to 900, mark + space from 2833 to 3276 */
	static const struct {
		uint32_t mark;
		uint32_t space;
		bool ok;
	} cases[] = {
		{2400, 875, true},  {2400, 876, false}, {2400, 877, false}, {2675, 600, true},
		{2676, 600, false}, {2000, 899, true},  {2000, 900, false}, {2533, 301, true},
		{2533, 300, false}, {2974, 301, true},  {2234, 600, true},  {2233, 600, false},
		{2232, 600, false}, {1935, 899, true},
	};
	uint32_t w[MAX_WORDS];

	ARRAY_FOR_EACH(cases, i) {
		memcpy(w, sony12_010015, sizeof(sony12_010015));
		w[0] = P(cases[i].mark);
		w[1] = S(cases[i].space);
		inject(w, ARRAY_SIZE(sony12_010015));
		if (cases[i].ok) {
			expect_sony_key(0x010015U);
		} else {
			expect_no_sony();
		}
	}
}

ZTEST(rc_sony, test_end_on_space)
{
	const struct rc_protocol *p = encoder_for(RC_PROTO_SONY20);
	uint32_t w[MAX_WORDS];
	uint32_t frame_us = 0U;
	int n;

	/* The longest frame repeated at 45 ms leaves a 6600 us gap and no timeout word */
	n = p->encode(RC_PROTO_SONY20, 0x1fff7fU, 0U, w, ARRAY_SIZE(w) / 2U);
	zassert_true(n > 0);
	for (int i = 0; i < n; i++) {
		frame_us += rc_raw_duration_us(w[i]);
	}
	zassert_equal(frame_us, 38400U);

	w[n] = S(45000U - frame_us);
	memcpy(&w[n + 1], w, n * sizeof(w[0]));
	inject(w, (2 * n) + 1);
	expect_event(RC_PROTO_SONY20, 0x1fff7fU, 0U);
	expect_event(RC_PROTO_SONY20, 0x1fff7fU, RC_EVENT_FLAG_REPEAT);
	expect_event(RC_PROTO_SONY20, 0x1fff7fU, RC_EVENT_FLAG_RELEASE);
}

ZTEST(rc_sony, test_repeat_period)
{
	const struct rc_tx_req req = {.proto = RC_PROTO_SONY12, .repeats = 2U, .scancode = 0x10U};

	tx_n = 0;
	rc_loopback_tx_cb_set(dev, tx_cb);
	zassert_ok(rc_send_sync(dev, &req, K_SECONDS(1)));
	zassert_equal(tx_n, 3U);
	for (size_t i = 1; i < tx_n; i++) {
		int64_t ms = k_ticks_to_ms_near64(tx_ticks[i] - tx_ticks[i - 1]);

		zassert_true((ms >= 45) && (ms <= 46), "frame %zu after %lld ms", i, ms);
	}

	expect_event(RC_PROTO_SONY12, 0x10U, 0U);
	expect_event(RC_PROTO_SONY12, 0x10U, RC_EVENT_FLAG_REPEAT);
	expect_event(RC_PROTO_SONY12, 0x10U, RC_EVENT_FLAG_REPEAT);
	expect_event(RC_PROTO_SONY12, 0x10U, RC_EVENT_FLAG_RELEASE);
}

ZTEST(rc_sony, test_truncated)
{
	inject(sony20_1aa57f, 20);
	expect_none();
	inject(sony12_010015, ARRAY_SIZE(sony12_010015));
	expect_key(RC_PROTO_SONY12, 0x010015U);
}

ZTEST(rc_sony, test_cross_protocol)
{
	static const struct {
		uint16_t proto;
		uint32_t scancode;
	} keys[] = {
		{RC_PROTO_NEC, 0x1234U},         {RC_PROTO_NECX, 0x123456U},
		{RC_PROTO_RC5, 0x1927U},         {RC_PROTO_RC5X_20, 0x0a4d15U},
		{RC_PROTO_RC6_0, 0xa37aU},       {RC_PROTO_RC6_6A_24, 0x123456U},
		{RC_PROTO_RC6_MCE, 0x800f0410U}, {RC_PROTO_SONY12, 0x010015U},
		{RC_PROTO_SONY15, 0xa50042U},    {RC_PROTO_SONY20, 0x1fff7fU},
	};
	uint32_t w[MAX_WORDS];

	/* Every frame decodes once, by its own decoder only */
	ARRAY_FOR_EACH(keys, i) {
		const struct rc_protocol *p = encoder_for(keys[i].proto);
		int n;

		zassert_not_null(p, "proto %u", keys[i].proto);
		n = p->encode(keys[i].proto, keys[i].scancode, 0U, w, ARRAY_SIZE(w));
		zassert_true(n > 0);
		inject(w, n);
		expect_key(keys[i].proto, keys[i].scancode);
		expect_none();
	}
}

ZTEST_SUITE(rc_sony, NULL, NULL, sony_before, sony_after, NULL);
