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

#define FRAME_WORDS 67
#define EVT_WAIT    K_MSEC(400)
#define QUIET_MS    200
#define BIT0_SPACE  3

static const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(rc_nec0));

K_MSGQ_DEFINE(evt_q, sizeof(struct rc_event), 16, 4);

static void evt_cb(const struct device *d, const struct rc_event *evt, void *user_data)
{
	ARG_UNUSED(d);
	ARG_UNUSED(user_data);

	(void)k_msgq_put(&evt_q, evt, K_NO_WAIT);
}

RC_CALLBACK_DEFINE(DEVICE_DT_GET(DT_NODELABEL(rc_nec0)), evt_cb, NULL);

static uint32_t nec_bits(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3)
{
	return b0 | ((uint32_t)b1 << 8) | ((uint32_t)b2 << 16) | ((uint32_t)b3 << 24);
}

static void build(uint32_t *w, uint32_t bits, int pulse_adj, int space_adj)
{
	size_t n = 0;

	w[n++] = RC_RAW_PULSE_US(9000 + pulse_adj);
	w[n++] = RC_RAW_SPACE_US(4500 + space_adj);
	for (int i = 0; i < 32; i++) {
		w[n++] = RC_RAW_PULSE_US(563 + pulse_adj);
		w[n++] = RC_RAW_SPACE_US((((bits >> i) & 1U) ? 1688 : 563) + space_adj);
	}
	w[n++] = RC_RAW_PULSE_US(563 + pulse_adj);
}

static void inject(const uint32_t *w, size_t count)
{
	zassert_ok(rc_loopback_inject(dev, w, count));
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

static void expect_key(const uint32_t *w, uint16_t proto, uint32_t scancode)
{
	inject(w, FRAME_WORDS);
	expect_event(proto, scancode, 0U);
	expect_event(proto, scancode, RC_EVENT_FLAG_RELEASE);
}

static const struct rc_protocol *nec_protocol(void)
{
	STRUCT_SECTION_FOREACH(rc_protocol, p) {
		if (strcmp(p->name, "nec") == 0) {
			return p;
		}
	}

	return NULL;
}

static void rc_nec_before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_msleep(QUIET_MS);
	k_msgq_purge(&evt_q);
	zassert_ok(rc_rx_enable(dev, true));
}

static void rc_nec_after(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(rc_rx_enable(dev, false));
}

ZTEST(rc_nec, test_round_trip)
{
	static const struct rc_tx_req reqs[] = {
		{.proto = RC_PROTO_NEC, .scancode = 0x1234U},
		{.proto = RC_PROTO_NECX, .scancode = 0x123456U},
		{.proto = RC_PROTO_NEC32, .scancode = 0x12345678U},
	};

	ARRAY_FOR_EACH(reqs, i) {
		zassert_ok(rc_send_sync(dev, &reqs[i], K_SECONDS(1)));
		expect_event(reqs[i].proto, reqs[i].scancode, 0U);
		expect_event(reqs[i].proto, reqs[i].scancode, RC_EVENT_FLAG_RELEASE);
	}
}

ZTEST(rc_nec, test_encode)
{
	const struct rc_protocol *p = nec_protocol();
	uint32_t want[FRAME_WORDS];
	uint32_t got[FRAME_WORDS];

	zassert_not_null(p);
	build(want, nec_bits(0x12, 0xed, 0x34, 0xcb), 0, 0);
	zassert_equal(p->encode(RC_PROTO_NEC, 0x1234U, 0U, got, ARRAY_SIZE(got)), FRAME_WORDS);
	zassert_mem_equal(got, want, sizeof(want));

	zassert_equal(p->encode(RC_PROTO_NEC, 0x10000U, 0U, got, ARRAY_SIZE(got)), -EINVAL);
	zassert_equal(p->encode(RC_PROTO_NECX, 0x1000000U, 0U, got, ARRAY_SIZE(got)), -EINVAL);
	zassert_equal(p->encode(RC_PROTO_RC5, 0x1234U, 0U, got, ARRAY_SIZE(got)), -EINVAL);
	zassert_equal(p->encode(RC_PROTO_NEC, 0x1234U, 0U, got, FRAME_WORDS - 1), -ENOBUFS);

	zassert_equal(p->encode_repeat(RC_PROTO_NEC, 0U, 0U, got, ARRAY_SIZE(got)), 3);
	zassert_equal(got[0], RC_RAW_PULSE_US(9000));
	zassert_equal(got[1], RC_RAW_SPACE_US(2250));
	zassert_equal(got[2], RC_RAW_PULSE_US(563));
	zassert_equal(p->encode_repeat(RC_PROTO_NEC, 0U, 0U, got, 2), -ENOBUFS);
}

ZTEST(rc_nec, test_repeat)
{
	static const struct rc_tx_req req = {
		.proto = RC_PROTO_NEC,
		.repeats = 2U,
		.scancode = 0x0102U,
	};

	zassert_ok(rc_send_sync(dev, &req, K_SECONDS(1)));
	expect_event(RC_PROTO_NEC, 0x0102U, 0U);
	expect_event(RC_PROTO_NEC, 0x0102U, RC_EVENT_FLAG_REPEAT);
	expect_event(RC_PROTO_NEC, 0x0102U, RC_EVENT_FLAG_REPEAT);
	expect_event(RC_PROTO_NEC, 0x0102U, RC_EVENT_FLAG_RELEASE);
}

ZTEST(rc_nec, test_repeat_no_key)
{
	const uint32_t rep[] = {RC_RAW_PULSE_US(9000), RC_RAW_SPACE_US(2250), RC_RAW_PULSE_US(563)};

	inject(rep, ARRAY_SIZE(rep));
	expect_none();
}

ZTEST(rc_nec, test_repeat_after_release)
{
	const uint32_t rep[] = {RC_RAW_PULSE_US(9000), RC_RAW_SPACE_US(2250), RC_RAW_PULSE_US(563)};
	uint32_t w[FRAME_WORDS];

	build(w, nec_bits(0x12, 0xed, 0x34, 0xcb), 0, 0);
	expect_key(w, RC_PROTO_NEC, 0x1234U);
	inject(rep, ARRAY_SIZE(rep));
	expect_none();
}

ZTEST(rc_nec, test_bad_leader)
{
	uint32_t w[FRAME_WORDS];

	build(w, nec_bits(0x12, 0xed, 0x34, 0xcb), 0, 0);
	w[0] = RC_RAW_PULSE_US(7000);
	inject(w, FRAME_WORDS);
	expect_none();

	w[0] = RC_RAW_PULSE_US(9000);
	expect_key(w, RC_PROTO_NEC, 0x1234U);
}

ZTEST(rc_nec, test_bit_out_of_tolerance)
{
	uint32_t w[FRAME_WORDS];

	build(w, nec_bits(0x12, 0xed, 0x34, 0xcb), 0, 0);
	w[2 + (2 * 10) + 1] = RC_RAW_SPACE_US(1100);
	inject(w, FRAME_WORDS);
	expect_none();
}

ZTEST(rc_nec, test_truncated)
{
	uint32_t w[FRAME_WORDS];

	build(w, nec_bits(0x12, 0xed, 0x34, 0xcb), 0, 0);
	inject(w, 2 + (2 * 10));
	expect_none();

	build(w, nec_bits(0x56, 0xa9, 0x78, 0x87), 0, 0);
	expect_key(w, RC_PROTO_NEC, 0x5678U);
}

ZTEST(rc_nec, test_address_inverse)
{
	uint32_t w[FRAME_WORDS];

	build(w, nec_bits(0x12, 0xed, 0x34, 0xcb), 0, 0);
	expect_key(w, RC_PROTO_NEC, 0x1234U);

	build(w, nec_bits(0x12, 0x34, 0x56, 0xa9), 0, 0);
	expect_key(w, RC_PROTO_NECX, 0x123456U);

	build(w, nec_bits(0x12, 0xed, 0x34, 0x00), 0, 0);
	expect_key(w, RC_PROTO_NEC32, 0xed120034U);
}

ZTEST(rc_nec, test_stretched_receiver)
{
	uint32_t w[FRAME_WORDS];

	build(w, nec_bits(0x12, 0xed, 0x34, 0xcb), 150, -150);
	expect_key(w, RC_PROTO_NEC, 0x1234U);
}

ZTEST(rc_nec, test_bit_boundaries)
{
	uint32_t w[FRAME_WORDS];

	build(w, nec_bits(0x12, 0xed, 0x34, 0xcb), 0, 0);
	w[BIT0_SPACE] = RC_RAW_SPACE_US(844);
	expect_key(w, RC_PROTO_NEC, 0x1234U);

	w[BIT0_SPACE] = RC_RAW_SPACE_US(845);
	inject(w, FRAME_WORDS);
	expect_none();

	build(w, nec_bits(0x13, 0xec, 0x34, 0xcb), 0, 0);
	w[BIT0_SPACE] = RC_RAW_SPACE_US(1406);
	inject(w, FRAME_WORDS);
	expect_none();

	w[BIT0_SPACE] = RC_RAW_SPACE_US(1407);
	expect_key(w, RC_PROTO_NEC, 0x1334U);
}

ZTEST_SUITE(rc_nec, NULL, NULL, rc_nec_before, rc_nec_after, NULL);
