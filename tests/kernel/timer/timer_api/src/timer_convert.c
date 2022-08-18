/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/types.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/random/rand32.h>

#define NUM_RANDOM 100

enum units { UNIT_ticks, UNIT_cyc, UNIT_ms, UNIT_us, UNIT_ns };

enum round { ROUND_floor, ROUND_ceil, ROUND_near };

static const char *const round_s[] = {
	[ROUND_floor] = "floor",
	[ROUND_ceil] = "ceil",
	[ROUND_near] = "near",
};

struct test_rec {
	enum units src;
	enum units dst;
	int precision; /* 32 or 64 */
	enum round round;
	void *func;
};

#define TESTREC(src, dst, round, prec) { \
		UNIT_##src, UNIT_##dst, prec, ROUND_##round,		\
			(void *)k_##src##_to_##dst##_##round##prec	\
	}								\

static struct test_rec tests[] = {
	 TESTREC(ms, cyc, floor, 32),
	 TESTREC(ms, cyc, floor, 64),
	 TESTREC(ms, cyc, near, 32),
	 TESTREC(ms, cyc, near, 64),
	 TESTREC(ms, cyc, ceil, 32),
	 TESTREC(ms, cyc, ceil, 64),
	 TESTREC(ms, ticks, floor, 32),
	 TESTREC(ms, ticks, floor, 64),
	 TESTREC(ms, ticks, near, 32),
	 TESTREC(ms, ticks, near, 64),
	 TESTREC(ms, ticks, ceil, 32),
	 TESTREC(ms, ticks, ceil, 64),
	 TESTREC(us, cyc, floor, 32),
	 TESTREC(us, cyc, floor, 64),
	 TESTREC(us, cyc, near, 32),
	 TESTREC(us, cyc, near, 64),
	 TESTREC(us, cyc, ceil, 32),
	 TESTREC(us, cyc, ceil, 64),
	 TESTREC(us, ticks, floor, 32),
	 TESTREC(us, ticks, floor, 64),
	 TESTREC(us, ticks, near, 32),
	 TESTREC(us, ticks, near, 64),
	 TESTREC(us, ticks, ceil, 32),
	 TESTREC(us, ticks, ceil, 64),
	 TESTREC(cyc, ms, floor, 32),
	 TESTREC(cyc, ms, floor, 64),
	 TESTREC(cyc, ms, near, 32),
	 TESTREC(cyc, ms, near, 64),
	 TESTREC(cyc, ms, ceil, 32),
	 TESTREC(cyc, ms, ceil, 64),
	 TESTREC(cyc, us, floor, 32),
	 TESTREC(cyc, us, floor, 64),
	 TESTREC(cyc, us, near, 32),
	 TESTREC(cyc, us, near, 64),
	 TESTREC(cyc, us, ceil, 32),
	 TESTREC(cyc, us, ceil, 64),
	 TESTREC(cyc, ticks, floor, 32),
	 TESTREC(cyc, ticks, floor, 64),
	 TESTREC(cyc, ticks, near, 32),
	 TESTREC(cyc, ticks, near, 64),
	 TESTREC(cyc, ticks, ceil, 32),
	 TESTREC(cyc, ticks, ceil, 64),
	 TESTREC(ticks, ms, floor, 32),
	 TESTREC(ticks, ms, floor, 64),
	 TESTREC(ticks, ms, near, 32),
	 TESTREC(ticks, ms, near, 64),
	 TESTREC(ticks, ms, ceil, 32),
	 TESTREC(ticks, ms, ceil, 64),
	 TESTREC(ticks, us, floor, 32),
	 TESTREC(ticks, us, floor, 64),
	 TESTREC(ticks, us, near, 32),
	 TESTREC(ticks, us, near, 64),
	 TESTREC(ticks, us, ceil, 32),
	 TESTREC(ticks, us, ceil, 64),
	 TESTREC(ticks, cyc, floor, 32),
	 TESTREC(ticks, cyc, floor, 64),
	 TESTREC(ticks, cyc, near, 32),
	 TESTREC(ticks, cyc, near, 64),
	 TESTREC(ticks, cyc, ceil, 32),
	 TESTREC(ticks, cyc, ceil, 64),
	 TESTREC(ns, cyc, floor, 32),
	 TESTREC(ns, cyc, floor, 64),
	 TESTREC(ns, cyc, near, 32),
	 TESTREC(ns, cyc, near, 64),
	 TESTREC(ns, cyc, ceil, 32),
	 TESTREC(ns, cyc, ceil, 64),
	 TESTREC(ns, ticks, floor, 32),
	 TESTREC(ns, ticks, floor, 64),
	 TESTREC(ns, ticks, near, 32),
	 TESTREC(ns, ticks, near, 64),
	 TESTREC(ns, ticks, ceil, 32),
	 TESTREC(ns, ticks, ceil, 64),
	 TESTREC(cyc, ns, floor, 32),
	 TESTREC(cyc, ns, floor, 64),
	 TESTREC(cyc, ns, near, 32),
	 TESTREC(cyc, ns, near, 64),
	 TESTREC(cyc, ns, ceil, 32),
	 TESTREC(cyc, ns, ceil, 64),
	 TESTREC(ticks, ns, floor, 32),
	 TESTREC(ticks, ns, floor, 64),
	 TESTREC(ticks, ns, near, 32),
	 TESTREC(ticks, ns, near, 64),
	 TESTREC(ticks, ns, ceil, 32),
	 TESTREC(ticks, ns, ceil, 64),
	};

uint32_t get_hz(enum units u)
{
	if (u == UNIT_ticks) {
		return CONFIG_SYS_CLOCK_TICKS_PER_SEC;
	} else if (u == UNIT_cyc) {
		return sys_clock_hw_cycles_per_sec();
	} else if (u == UNIT_ms) {
		return 1000;
	} else if (u == UNIT_us) {
		return 1000000;
	} else if (u == UNIT_ns) {
		return 1000000000;
	}
	__ASSERT(0, "");
	return 0;
}

static void test_conversion(struct test_rec *t, uint64_t val)
{
	uint32_t from_hz = get_hz(t->src), to_hz = get_hz(t->dst);
	uint64_t result;

	if (t->precision == 32) {
		uint32_t (*convert)(uint32_t) = (uint32_t (*)(uint32_t)) t->func;

		result = convert((uint32_t) val);

		/* If the input value legitimately overflows, then
		 * there is nothing to test
		 */
		if ((val * to_hz) >= ((((uint64_t)from_hz) << 32))) {
			return;
		}
	} else {
		uint64_t (*convert)(uint64_t) = (uint64_t (*)(uint64_t)) t->func;

		result = convert(val);
	}

	/* We expect the ideal result to be equal to "val * to_hz /
	 * from_hz", but that division is the source of precision
	 * issues.  So reexpress our equation as:
	 *
	 *    val * to_hz ==? result * from_hz
	 *              0 ==? val * to_hz - result * from_hz
	 *
	 * The difference is allowed to be in the range [0:from_hz) if
	 * we are rounding down, from (-from_hz:0] if we are rounding
	 * up, or [-from_hz/2:from_hz/2] if we are rounding to the
	 * nearest.
	 */
	int64_t diff = (int64_t)(val * to_hz - result * from_hz);
	int64_t maxdiff, mindiff;

	if (t->round == ROUND_floor) {
		maxdiff = from_hz - 1;
		mindiff = 0;
	} else if (t->round == ROUND_ceil) {
		maxdiff = 0;
		mindiff = -(int64_t)(from_hz-1);
	} else {
		maxdiff = from_hz/2;
		mindiff = -(int64_t)(from_hz/2);
	}

	zassert_true(diff <= maxdiff && diff >= mindiff,
		     "Convert %llu (%llx) from %u Hz to %u Hz %u-bit %s\n"
		     "result %llu (%llx) diff %lld (%llx) should be in [%lld:%lld]",
		     val, val, from_hz, to_hz, t->precision, round_s[t->round],
		     result, result, diff, diff, mindiff, maxdiff);
}

ZTEST(timer_api, test_time_conversions)
{
	for (int i = 0; i < ARRAY_SIZE(tests); i++) {
		test_conversion(&tests[i], 0);
		test_conversion(&tests[i], 1);
		test_conversion(&tests[i], 0x7fffffff);
		test_conversion(&tests[i], 0x80000000);
		test_conversion(&tests[i], 0xfffffff0);
		if (tests[i].precision == 64) {
			test_conversion(&tests[i], 0xffffffff);
			test_conversion(&tests[i], 0x100000000ULL);
		}

		for (int j = 0; j < NUM_RANDOM; j++) {
			test_conversion(&tests[i], sys_rand32_get());
		}
	}
}
