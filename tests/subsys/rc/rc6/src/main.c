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
#define T         444U
#define EVT_WAIT  K_MSEC(400)
#define QUIET_MS  200
#define MAX_WORDS 96

static const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(rc_rc6_0));

/* Built from the bit strings by hand: leader, start bit, mode, trailer bit, body */
static const uint32_t rc6_a37a[] = {
	P(2664), S(888), P(444), S(888), P(444), S(444), P(444), S(444), P(444), S(888), P(1332),
	S(888),  P(888), S(888), P(444), S(444), P(444), S(444), P(888), S(444), P(444), S(888),
	P(888),  S(444), P(444), S(444), P(444), S(444), P(444), S(888), P(888), S(888), P(444),
};
static const uint32_t rc6_a37a_t[] = {
	P(2664), S(888), P(444), S(888), P(444), S(444), P(444), S(444), P(1332), S(888), P(444),
	S(888),  P(888), S(888), P(444), S(444), P(444), S(444), P(888), S(444),  P(444), S(888),
	P(888),  S(444), P(444), S(444), P(444), S(444), P(444), S(888), P(888),  S(888), P(444),
};
static const uint32_t rc6a20_12345[] = {
	P(2664), S(888), P(444), S(444), P(444), S(444), P(444), S(888), P(444), S(888),
	P(888),  S(444), P(444), S(444), P(444), S(444), P(888), S(888), P(444), S(444),
	P(888),  S(888), P(444), S(444), P(444), S(444), P(888), S(444), P(444), S(888),
	P(888),  S(888), P(444), S(444), P(444), S(444), P(888), S(888), P(888),
};

K_MSGQ_DEFINE(evt_q, sizeof(struct rc_event), 16, 4);

static void evt_cb(const struct device *d, const struct rc_event *evt, void *user_data)
{
	ARG_UNUSED(d);
	ARG_UNUSED(user_data);

	(void)k_msgq_put(&evt_q, evt, K_NO_WAIT);
}

RC_CALLBACK_DEFINE(DEVICE_DT_GET(DT_NODELABEL(rc_rc6_0)), evt_cb, NULL);

static const struct rc_protocol *rc6_protocol(void)
{
	STRUCT_SECTION_FOREACH(rc_protocol, p) {
		if (strcmp(p->name, "rc6") == 0) {
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

/* Other decoders may take a frame that RC6 rejects */
static void expect_no_rc6(void)
{
	struct rc_event evt;

	while (k_msgq_get(&evt_q, &evt, K_MSEC(QUIET_MS)) == 0) {
		zassert_false((evt.proto >= RC_PROTO_RC6_0) && (evt.proto <= RC_PROTO_RC6_MCE),
			      "RC6 event proto %u", evt.proto);
	}
}

static void expect_key(uint16_t proto, uint32_t scancode, uint16_t toggle)
{
	expect_event(proto, scancode, toggle);
	expect_event(proto, scancode, toggle | RC_EVENT_FLAG_RELEASE);
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

/* A frame with any header, trailer bit and body length */
static size_t build(uint32_t *w, uint32_t mode, uint32_t tr, uint32_t body, uint8_t nbits)
{
	struct rc_raw_writer wr;

	rc_raw_writer_init(&wr, w, MAX_WORDS);
	rc_raw_put(&wr, true, 6U * T);
	rc_raw_put(&wr, false, 2U * T);
	rc_gen_manchester(&wr, BIT(3) | mode, 4U, T, true);
	rc_gen_manchester(&wr, tr, 1U, 2U * T, true);
	rc_gen_manchester(&wr, body, nbits, T, true);

	return (size_t)rc_raw_end(&wr);
}

static uint16_t send_key(uint16_t proto, uint32_t scancode)
{
	const struct rc_tx_req req = {.proto = proto, .scancode = scancode};
	struct rc_event evt;

	zassert_ok(rc_send_sync(dev, &req, K_SECONDS(1)));
	zassert_ok(k_msgq_get(&evt_q, &evt, EVT_WAIT), "no event");
	zassert_equal(evt.proto, proto, "proto %u", evt.proto);
	zassert_equal(evt.scancode, scancode, "scancode 0x%x", evt.scancode);
	expect_event(proto, scancode, evt.flags | RC_EVENT_FLAG_RELEASE);

	return evt.flags & RC_EVENT_FLAG_TOGGLE;
}

static void rc6_before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_msleep(QUIET_MS);
	k_msgq_purge(&evt_q);
	zassert_ok(rc_rx_enable(dev, true));
}

static void rc6_after(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(rc_rx_enable(dev, false));
}

ZTEST(rc_rc6, test_round_trip)
{
	static const struct {
		uint16_t proto;
		uint32_t scancode;
	} keys[] = {
		{RC_PROTO_RC6_0, 0xa37aU},         {RC_PROTO_RC6_0, 0x0000U},
		{RC_PROTO_RC6_0, 0xffffU},         {RC_PROTO_RC6_6A_20, 0x12345U},
		{RC_PROTO_RC6_6A_20, 0xfffffU},    {RC_PROTO_RC6_6A_24, 0x123456U},
		{RC_PROTO_RC6_6A_32, 0x12345678U}, {RC_PROTO_RC6_6A_32, 0xffffffffU},
		{RC_PROTO_RC6_MCE, 0x800f0410U},   {RC_PROTO_RC6_MCE, 0x80347fffU},
		{RC_PROTO_RC6_MCE, 0x80460001U},
	};

	ARRAY_FOR_EACH(keys, i) {
		(void)send_key(keys[i].proto, keys[i].scancode);
	}
}

ZTEST(rc_rc6, test_toggle)
{
	uint16_t first = send_key(RC_PROTO_RC6_0, 0x0102U);

	zassert_not_equal(first, send_key(RC_PROTO_RC6_0, 0x0102U), "toggle not flipped");
	first = send_key(RC_PROTO_RC6_MCE, 0x800f0410U);
	zassert_not_equal(first, send_key(RC_PROTO_RC6_MCE, 0x800f0410U), "toggle not flipped");
	zassert_equal(send_key(RC_PROTO_RC6_6A_20, 0x1U), 0U);
	zassert_equal(send_key(RC_PROTO_RC6_6A_20, 0x1U), 0U);
}

ZTEST(rc_rc6, test_decode_vectors)
{
	inject(rc6_a37a, ARRAY_SIZE(rc6_a37a));
	expect_key(RC_PROTO_RC6_0, 0xa37aU, 0U);
	inject(rc6_a37a_t, ARRAY_SIZE(rc6_a37a_t));
	expect_key(RC_PROTO_RC6_0, 0xa37aU, RC_EVENT_FLAG_TOGGLE);
	inject(rc6a20_12345, ARRAY_SIZE(rc6a20_12345));
	expect_key(RC_PROTO_RC6_6A_20, 0x12345U, 0U);
}

ZTEST(rc_rc6, test_encode)
{
	const struct rc_protocol *p = rc6_protocol();
	uint32_t got[MAX_WORDS];

	zassert_not_null(p);
	zassert_equal(p->encode(RC_PROTO_RC6_0, 0xa37aU, 0U, got, ARRAY_SIZE(got)),
		      ARRAY_SIZE(rc6_a37a));
	zassert_mem_equal(got, rc6_a37a, sizeof(rc6_a37a));
	zassert_equal(
		p->encode(RC_PROTO_RC6_0, 0xa37aU, RC_EVENT_FLAG_TOGGLE, got, ARRAY_SIZE(got)),
		ARRAY_SIZE(rc6_a37a_t));
	zassert_mem_equal(got, rc6_a37a_t, sizeof(rc6_a37a_t));
	zassert_equal(
		p->encode(RC_PROTO_RC6_6A_20, 0x12345U, RC_EVENT_FLAG_TOGGLE, got, ARRAY_SIZE(got)),
		ARRAY_SIZE(rc6a20_12345));
	zassert_mem_equal(got, rc6a20_12345, sizeof(rc6a20_12345));

	zassert_equal(p->encode(RC_PROTO_RC6_0, 0x10000U, 0U, got, ARRAY_SIZE(got)), -EINVAL);
	zassert_equal(p->encode(RC_PROTO_RC6_6A_20, 0x100000U, 0U, got, ARRAY_SIZE(got)), -EINVAL);
	zassert_equal(p->encode(RC_PROTO_RC6_6A_24, 0x1000000U, 0U, got, ARRAY_SIZE(got)), -EINVAL);
	zassert_equal(p->encode(RC_PROTO_RC6_6A_32, 0x800f0410U, 0U, got, ARRAY_SIZE(got)),
		      -EINVAL);
	zassert_equal(p->encode(RC_PROTO_RC6_MCE, 0x12340410U, 0U, got, ARRAY_SIZE(got)), -EINVAL);
	zassert_equal(p->encode(RC_PROTO_RC6_MCE, 0x800f8410U, 0U, got, ARRAY_SIZE(got)), -EINVAL);
	zassert_equal(p->encode(RC_PROTO_RC5, 0x1U, 0U, got, ARRAY_SIZE(got)), -EINVAL);
	zassert_equal(p->encode(RC_PROTO_RC6_0, 0xa37aU, 0U, got, ARRAY_SIZE(rc6_a37a) - 1U),
		      -ENOBUFS);
	zassert_is_null(p->encode_repeat);
}

ZTEST(rc_rc6, test_unit_boundaries)
{
	/* Word 2 is 1t, word 3 is 2t, word 10 is 3t; classes split at 666, 1110, 1554 us */
	static const struct {
		size_t word;
		uint32_t us;
		bool ok;
	} cases[] = {
		{2, 665, true},    {2, 666, false},  {2, 667, false},   {3, 667, true},
		{3, 665, false},   {3, 1109, true},  {3, 1111, false},  {10, 1111, true},
		{10, 1109, false}, {10, 1553, true}, {10, 1555, false}, {0, 3107, true},
		{0, 3108, false},
	};

	ARRAY_FOR_EACH(cases, i) {
		inject_changed(rc6_a37a, ARRAY_SIZE(rc6_a37a), cases[i].word, cases[i].us);
		if (cases[i].ok) {
			expect_key(RC_PROTO_RC6_0, 0xa37aU, 0U);
		} else {
			expect_none();
		}
	}
}

ZTEST(rc_rc6, test_leader_boundaries)
{
	/* Mark from 2220 to 3108, space from 666 to 1110, mark + space above 3276 */
	static const struct {
		uint32_t mark;
		uint32_t space;
		bool ok;
	} cases[] = {
		{2400, 877, true},  {2400, 876, false},  {2400, 875, false},
		{2221, 1109, true}, {2220, 1109, false}, {2664, 667, true},
		{2664, 666, false}, {2664, 1109, true},  {2664, 1110, false},
	};
	uint32_t w[MAX_WORDS];

	ARRAY_FOR_EACH(cases, i) {
		memcpy(w, rc6_a37a, sizeof(rc6_a37a));
		w[0] = P(cases[i].mark);
		w[1] = S(cases[i].space);
		inject(w, ARRAY_SIZE(rc6_a37a));
		if (cases[i].ok) {
			expect_key(RC_PROTO_RC6_0, 0xa37aU, 0U);
		} else {
			expect_no_rc6();
		}
	}
}

ZTEST(rc_rc6, test_bad_frames)
{
	uint32_t w[MAX_WORDS];

	/* 6A with 22 bits, mode 1, and 6A with the trailer bit set */
	inject(w, build(w, 6U, 0U, 0x12345U, 22U));
	expect_none();
	inject(w, build(w, 1U, 0U, 0x1234U, 16U));
	expect_none();
	inject(w, build(w, 6U, 1U, 0x12345U, 20U));
	expect_none();
	inject(w, build(w, 6U, 0U, 0x12345U, 20U));
	expect_key(RC_PROTO_RC6_6A_20, 0x12345U, 0U);
}

ZTEST(rc_rc6, test_truncated)
{
	uint32_t w[MAX_WORDS];

	inject(rc6_a37a, 12);
	expect_none();

	/* A broken frame directly followed by a good one */
	memcpy(w, rc6_a37a, 12 * sizeof(w[0]));
	memcpy(&w[12], rc6_a37a_t, sizeof(rc6_a37a_t));
	w[11] = S(888U + 3U * T);
	inject(w, 12 + ARRAY_SIZE(rc6_a37a_t));
	expect_key(RC_PROTO_RC6_0, 0xa37aU, RC_EVENT_FLAG_TOGGLE);
}

ZTEST_SUITE(rc_rc6, NULL, NULL, rc6_before, rc6_after, NULL);
