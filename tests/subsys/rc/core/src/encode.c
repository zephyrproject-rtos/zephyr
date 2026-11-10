/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/rc/rc.h>
#include <zephyr/rc/rc_protocol.h>
#include <zephyr/ztest.h>

#define P(us) RC_RAW_PULSE_US(us)
#define S(us) RC_RAW_SPACE_US(us)

static void expect_words(const uint32_t *got, int n, const uint32_t *want, size_t count)
{
	zassert_equal(n, (int)count, "%d words", n);
	zassert_mem_equal(got, want, count * sizeof(uint32_t));
}

ZTEST(rc_encode, test_put_merge)
{
	const uint32_t want[] = {P(300), S(500), P(70)};
	uint32_t words[8];
	struct rc_raw_writer w;

	rc_raw_writer_init(&w, words, ARRAY_SIZE(words));
	rc_raw_put(&w, false, 100);
	rc_raw_put(&w, true, 100);
	rc_raw_put(&w, true, 200);
	rc_raw_put(&w, false, 0);
	rc_raw_put(&w, false, 500);
	rc_raw_put(&w, true, 70);
	rc_raw_put(&w, false, 900);
	expect_words(words, rc_raw_end(&w), want, ARRAY_SIZE(want));
}

ZTEST(rc_encode, test_put_clamp)
{
	uint32_t words[2];
	struct rc_raw_writer w;

	rc_raw_writer_init(&w, words, ARRAY_SIZE(words));
	rc_raw_put(&w, true, RC_RAW_DURATION_MASK);
	rc_raw_put(&w, true, 10);
	zassert_equal(rc_raw_end(&w), 1);
	zassert_equal(words[0], P(RC_RAW_DURATION_MASK));
}

ZTEST(rc_encode, test_overflow)
{
	uint32_t words[3];
	struct rc_raw_writer w;

	rc_raw_writer_init(&w, words, ARRAY_SIZE(words));
	rc_raw_put(&w, true, 100);
	rc_raw_put(&w, false, 100);
	rc_raw_put(&w, true, 100);
	rc_raw_put(&w, true, 100);
	zassert_equal(rc_raw_end(&w), 3);

	rc_raw_writer_init(&w, words, ARRAY_SIZE(words));
	rc_raw_put(&w, true, 100);
	rc_raw_put(&w, false, 100);
	rc_raw_put(&w, true, 100);
	rc_raw_put(&w, false, 100);
	zassert_equal(rc_raw_end(&w), -ENOBUFS);
}

ZTEST(rc_encode, test_manchester)
{
	const uint32_t pulse_first[] = {P(100), S(200), P(100)};
	const uint32_t space_first[] = {P(200), S(200), P(100)};
	uint32_t words[8];
	struct rc_raw_writer w;

	/* 1 then 0: pulse space, space pulse */
	rc_raw_writer_init(&w, words, ARRAY_SIZE(words));
	rc_gen_manchester(&w, 0x2U, 2U, 100U, true);
	expect_words(words, rc_raw_end(&w), pulse_first, ARRAY_SIZE(pulse_first));

	/* 1, 0, 1 with 1 as space pulse: the leading space is dropped */
	rc_raw_writer_init(&w, words, ARRAY_SIZE(words));
	rc_gen_manchester(&w, 0x5U, 3U, 100U, false);
	rc_raw_put(&w, false, 300);
	expect_words(words, rc_raw_end(&w), space_first, ARRAY_SIZE(space_first));
}

ZTEST(rc_encode, test_pd)
{
	const struct rc_gen_pd_timings t = {
		.header_pulse = 900,
		.header_space = 450,
		.bit_pulse = 50,
		.bit_space = {50, 150},
		.trailer_pulse = 50,
	};
	struct rc_gen_pd_timings t_msb = t;
	const uint32_t lsb[] = {P(900), S(450), P(50), S(150), P(50), S(50), P(50)};
	const uint32_t msb[] = {P(900), S(450), P(50), S(50), P(50), S(150), P(50)};
	uint32_t words[8];
	struct rc_raw_writer w;

	t_msb.msb_first = true;
	rc_raw_writer_init(&w, words, ARRAY_SIZE(words));
	rc_gen_pd(&w, &t, 0x1U, 2U);
	expect_words(words, rc_raw_end(&w), lsb, ARRAY_SIZE(lsb));

	rc_raw_writer_init(&w, words, ARRAY_SIZE(words));
	rc_gen_pd(&w, &t_msb, 0x1U, 2U);
	expect_words(words, rc_raw_end(&w), msb, ARRAY_SIZE(msb));

	rc_raw_writer_init(&w, words, 6);
	rc_gen_pd(&w, &t, 0x1U, 2U);
	zassert_equal(rc_raw_end(&w), -ENOBUFS);
}

ZTEST(rc_encode, test_pl)
{
	const struct rc_gen_pl_timings t = {
		.header_pulse = 240,
		.bit_space = 60,
		.bit_pulse = {60, 120},
	};
	const uint32_t want[] = {P(240), S(60), P(60), S(60), P(120)};
	uint32_t words[8];
	struct rc_raw_writer w;

	rc_raw_writer_init(&w, words, ARRAY_SIZE(words));
	rc_gen_pl(&w, &t, 0x2U, 2U);
	expect_words(words, rc_raw_end(&w), want, ARRAY_SIZE(want));
}

ZTEST(rc_encode, test_32_bits)
{
	uint32_t words[2 * 32 + 1];
	struct rc_raw_writer w;
	int n;

	rc_raw_writer_init(&w, words, ARRAY_SIZE(words));
	rc_gen_manchester(&w, 0xffffffffU, 32U, 10U, true);
	n = rc_raw_end(&w);
	zassert_equal(n, 63);
	zassert_equal(words[0], P(10));
	zassert_equal(words[62], P(10));
}

ZTEST_SUITE(rc_encode, NULL, NULL, NULL, NULL, NULL);
