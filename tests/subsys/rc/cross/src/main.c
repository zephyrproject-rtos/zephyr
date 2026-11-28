/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/rc/rc.h>
#include <zephyr/rc/rc_protocol.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define MAX_WORDS    128
#define MAX_DECODERS 8
#define STATE_WORDS  16
#define SKEW_US      150U
#define RANDOM_N     10000U
#define MAX_PRINTS   10

struct decoder {
	const struct rc_protocol *p;
	uint64_t state[STATE_WORDS];
};

static struct decoder decoders[MAX_DECODERS];
static size_t num_decoders;
static uint32_t collisions;
static uint32_t misses;
static uint32_t frames;
static uint32_t rng = 0x2545f491U;

static uint32_t next_rand(void)
{
	rng ^= rng << 13;
	rng ^= rng >> 17;
	rng ^= rng << 5;

	return rng;
}

static bool in_family(const struct rc_protocol *p, uint16_t proto)
{
	return (proto >= p->proto_first) && (proto <= p->proto_last);
}

static const struct rc_protocol *encoder_for(uint16_t proto)
{
	STRUCT_SECTION_FOREACH(rc_protocol, p) {
		if ((p->encode != NULL) && in_family(p, proto)) {
			return p;
		}
	}

	return NULL;
}

static void report(const char *what, uint16_t proto, uint32_t scancode, uint16_t flags, bool skew,
		   const char *by, const struct rc_decode_result *res)
{
	if ((collisions + misses) <= MAX_PRINTS) {
		TC_PRINT("%s: proto %u scancode 0x%x flags %u skew %d -> %s proto %u 0x%x\n", what,
			 proto, scancode, flags, skew, by, res->proto, res->scancode);
	}
}

/* Feed one frame and its timeout word straight into every decoder */
static void check(uint16_t proto, uint32_t scancode, uint16_t flags, bool skew)
{
	const struct rc_protocol *enc = encoder_for(proto);
	/* Static, too large for the test thread stack */
	static uint32_t w[MAX_WORDS];
	static uint32_t again[MAX_WORDS];
	int n = enc->encode(proto, scancode, flags, w, MAX_WORDS);

	zassert_true(n > 0, "encode proto %u scancode 0x%x: %d", proto, scancode, n);
	frames++;

	for (size_t d = 0; d < num_decoders; d++) {
		const struct rc_protocol *p = decoders[d].p;
		struct rc_decode_result res = {0};
		uint32_t hits = 0U;
		bool own = in_family(p, proto);

		p->reset(decoders[d].state);
		for (int i = 0; i <= n; i++) {
			uint32_t word = (i < n) ? w[i] : (RC_RAW_TIMEOUT | RC_RAW_SPACE_US(20000U));
			uint32_t us = rc_raw_duration_us(word);

			if (skew && (i < n)) {
				word = rc_raw_is_pulse(word) ? RC_RAW_PULSE_US(us + SKEW_US)
							     : RC_RAW_SPACE_US(us - SKEW_US);
			}
			if (p->decode(decoders[d].state, word, &res) == 1) {
				hits++;
				if (!own) {
					collisions++;
					report("collision", proto, scancode, flags, skew, p->name,
					       &res);
				}
			}
		}

		if (!own) {
			continue;
		}
		/* The own decoder must report once, with a result that encodes the same frame */
		if ((hits != 1U) || !in_family(p, res.proto) ||
		    (enc->encode(res.proto, res.scancode, res.flags, again, MAX_WORDS) != n) ||
		    (memcmp(w, again, n * sizeof(w[0])) != 0)) {
			misses++;
			report("miss", proto, scancode, flags, skew, p->name, &res);
		}
	}
}

static void check_both(uint16_t proto, uint32_t scancode, uint16_t flags)
{
	check(proto, scancode, flags, false);
	check(proto, scancode, flags, true);
}

static void *cross_setup(void)
{
	STRUCT_SECTION_FOREACH(rc_protocol, p) {
		if (p->decode == NULL) {
			continue;
		}
		zassert_true(num_decoders < MAX_DECODERS);
		zassert_true(p->state_size <= sizeof(decoders[0].state));
		decoders[num_decoders++].p = p;
	}
	zassert_equal(num_decoders, 4U, "decoders %zu", num_decoders);

	return NULL;
}

static void cross_before(void *fixture)
{
	ARG_UNUSED(fixture);

	collisions = 0U;
	misses = 0U;
	frames = 0U;
}

static void cross_after(void *fixture)
{
	ARG_UNUSED(fixture);

	TC_PRINT("frames %u collisions %u misses %u\n", frames, collisions, misses);
}

static void expect_clean(void)
{
	zassert_equal(collisions, 0U, "%u collisions", collisions);
	zassert_equal(misses, 0U, "%u misses", misses);
}

ZTEST(rc_cross, test_rc5)
{
	for (uint32_t t = 0U; t < 2U; t++) {
		uint16_t flags = (t != 0U) ? RC_EVENT_FLAG_TOGGLE : 0U;

		for (uint32_t sys = 0U; sys < 32U; sys++) {
			for (uint32_t cmd = 0U; cmd < 128U; cmd++) {
				check_both(RC_PROTO_RC5, (sys << 8) | cmd, flags);
				for (uint32_t x = 0U; x < 64U; x++) {
					check_both(RC_PROTO_RC5X_20, (sys << 16) | (cmd << 8) | x,
						   flags);
				}
			}
		}
	}
	expect_clean();
}

ZTEST(rc_cross, test_rc6_0)
{
	for (uint32_t t = 0U; t < 2U; t++) {
		for (uint32_t sc = 0U; sc <= 0xffffU; sc++) {
			check_both(RC_PROTO_RC6_0, sc, (t != 0U) ? RC_EVENT_FLAG_TOGGLE : 0U);
		}
	}
	expect_clean();
}

ZTEST(rc_cross, test_rc6a)
{
	static const uint32_t mce[] = {0x800f0000U, 0x80340000U, 0x80460000U};

	for (uint32_t i = 0U; i < RANDOM_N; i++) {
		uint32_t sc;
		bool is_mce;

		check_both(RC_PROTO_RC6_6A_20, next_rand() & 0xfffffU, 0U);
		check_both(RC_PROTO_RC6_6A_24, next_rand() & 0xffffffU, 0U);
		do {
			sc = next_rand();
			is_mce = false;
			ARRAY_FOR_EACH(mce, j) {
				is_mce = is_mce || ((sc & 0xffff0000U) == mce[j]);
			}
		} while (is_mce);
		check_both(RC_PROTO_RC6_6A_32, sc, 0U);
		sc = mce[next_rand() % ARRAY_SIZE(mce)] | (next_rand() & 0x7fffU);
		check_both(RC_PROTO_RC6_MCE, sc,
			   ((next_rand() & 1U) != 0U) ? RC_EVENT_FLAG_TOGGLE : 0U);
	}
	expect_clean();
}

ZTEST(rc_cross, test_sony)
{
	for (uint32_t dev = 0U; dev < 256U; dev++) {
		for (uint32_t fn = 0U; fn < 128U; fn++) {
			if (dev < 32U) {
				check_both(RC_PROTO_SONY12, (dev << 16) | fn, 0U);
			}
			check_both(RC_PROTO_SONY15, (dev << 16) | fn, 0U);
		}
	}
	for (uint32_t i = 0U; i < RANDOM_N; i++) {
		check_both(RC_PROTO_SONY20, next_rand() & 0x1fff7fU, 0U);
	}
	expect_clean();
}

ZTEST(rc_cross, test_nec)
{
	for (uint32_t i = 0U; i < RANDOM_N; i++) {
		check_both(RC_PROTO_NEC, next_rand() & 0xffffU, 0U);
		check_both(RC_PROTO_NECX, next_rand() & 0xffffffU, 0U);
		check_both(RC_PROTO_NEC32, next_rand(), 0U);
	}
	expect_clean();
}

ZTEST_SUITE(rc_cross, NULL, cross_setup, cross_before, cross_after, NULL);
