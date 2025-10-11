/*
 * Copyright 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <time.h>

#include <zephyr/ztest.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/sys/util.h>

#undef CORRECTABLE
#define CORRECTABLE true

#undef UNCORRECTABLE
#define UNCORRECTABLE false

/* Initialize a struct timespec object from a tick count with additional nanoseconds */
#define SYS_TICKS_TO_TIMESPEC_PLUS_NSECS(ticks, ns)                                                \
	SYS_TIMESPEC(SYS_TICKS_TO_SECS(ticks) +                                                    \
			   (SYS_TICKS_TO_NSECS(ticks) + (uint64_t)(ns)) / NSEC_PER_SEC,            \
		   (SYS_TICKS_TO_NSECS(ticks) + (uint64_t)(ns)) % NSEC_PER_SEC)

/*
 * test spec for simple timespec validation
 *
 * If a timespec is expected to be valid, then invalid_ts and valid_ts are equal.
 *
 * If a timespec is expected to be invalid, then invalid_ts and valid_ts are not equal.
 */
struct ts_test_spec {
	struct timespec invalid_ts;
	struct timespec valid_ts;
	bool expect_valid;
	bool correctable;
};

#define DECL_VALID_TS_TEST(sec, nsec)                                                              \
	{                                                                                          \
		.invalid_ts = {(sec), (nsec)},                                                     \
		.valid_ts = {(sec), (nsec)},                                                       \
		.expect_valid = true,                                                              \
		.correctable = false,                                                              \
	}

/*
 * Invalid timespecs can usually be converted to valid ones by adding or subtracting
 * multiples of `NSEC_PER_SEC` from tv_nsec, and incrementing or decrementing tv_sec
 * proportionately, unless doing so would result in signed integer overflow.
 *
 * There are two particular corner cases:
 * 1. making tv_sec more negative would overflow the tv_sec field in the negative direction
 * 1. making tv_sec more positive would overflow the tv_sec field in the positive direction
 */
#define DECL_INVALID_TS_TEST(invalid_sec, invalid_nsec, valid_sec, valid_nsec, is_correctable)     \
	{                                                                                          \
		.valid_ts = {(valid_sec), (valid_nsec)},                                           \
		.invalid_ts = {(invalid_sec), (invalid_nsec)},                                     \
		.expect_valid = false,                                                             \
		.correctable = (is_correctable),                                                   \
	}

/*
 * Easily verifiable tests for both valid and invalid timespecs.
 */
static const struct ts_test_spec ts_tests[] = {
	/* Valid cases */
	DECL_VALID_TS_TEST(0, 0),
	DECL_VALID_TS_TEST(0, 1),
	DECL_VALID_TS_TEST(1, 0),
	DECL_VALID_TS_TEST(1, 1),
	DECL_VALID_TS_TEST(1, NSEC_PER_SEC - 1),
	DECL_VALID_TS_TEST(-1, 0),
	DECL_VALID_TS_TEST(-1, 1),
	DECL_VALID_TS_TEST(-1, 0),
	DECL_VALID_TS_TEST(-1, 1),
	DECL_VALID_TS_TEST(-1, NSEC_PER_SEC - 1),
	DECL_VALID_TS_TEST(SYS_TIME_T_MIN, 0),
	DECL_VALID_TS_TEST(SYS_TIME_T_MIN, NSEC_PER_SEC - 1),
	DECL_VALID_TS_TEST(SYS_TIME_T_MAX, 0),
	DECL_VALID_TS_TEST(SYS_TIME_T_MAX, NSEC_PER_SEC - 1),

	/* Correctable, invalid cases */
	DECL_INVALID_TS_TEST(0, -2LL * NSEC_PER_SEC + 1, -2, 1, CORRECTABLE),
	DECL_INVALID_TS_TEST(0, -2LL * NSEC_PER_SEC - 1, -3, NSEC_PER_SEC - 1, CORRECTABLE),
	DECL_INVALID_TS_TEST(0, -1LL * NSEC_PER_SEC - 1, -2, NSEC_PER_SEC - 1, CORRECTABLE),
	DECL_INVALID_TS_TEST(0, -1, -1, NSEC_PER_SEC - 1, CORRECTABLE),
	DECL_INVALID_TS_TEST(0, NSEC_PER_SEC, 1, 0, CORRECTABLE),
	DECL_INVALID_TS_TEST(0, NSEC_PER_SEC + 1, 1, 1, CORRECTABLE),
	DECL_INVALID_TS_TEST(1, -1, 0, NSEC_PER_SEC - 1, CORRECTABLE),
	DECL_INVALID_TS_TEST(1, NSEC_PER_SEC, 2, 0, CORRECTABLE),
	DECL_INVALID_TS_TEST(-1, -1, -2, NSEC_PER_SEC - 1, CORRECTABLE),
	DECL_INVALID_TS_TEST(0, NSEC_PER_SEC, 1, 0, CORRECTABLE),
	DECL_INVALID_TS_TEST(1, -1, 0, NSEC_PER_SEC - 1, CORRECTABLE),
	DECL_INVALID_TS_TEST(1, NSEC_PER_SEC, 2, 0, CORRECTABLE),
	DECL_INVALID_TS_TEST(SYS_TIME_T_MIN, NSEC_PER_SEC, SYS_TIME_T_MIN + 1, 0, CORRECTABLE),
	DECL_INVALID_TS_TEST(SYS_TIME_T_MAX, -1, SYS_TIME_T_MAX - 1, NSEC_PER_SEC - 1, CORRECTABLE),
	DECL_INVALID_TS_TEST(0, LONG_MIN, (int64_t)LONG_MIN / NSEC_PER_SEC - 1,
			     NSEC_PER_SEC + LONG_MIN % (long long)NSEC_PER_SEC, CORRECTABLE),
	DECL_INVALID_TS_TEST(0, LONG_MAX, LONG_MAX / NSEC_PER_SEC, LONG_MAX % NSEC_PER_SEC,
			     CORRECTABLE),

	/* Uncorrectable, invalid cases */
	DECL_INVALID_TS_TEST(SYS_TIME_T_MIN + 2, -2 * (int64_t)NSEC_PER_SEC - 1, 0, 0,
			     UNCORRECTABLE),
	DECL_INVALID_TS_TEST(SYS_TIME_T_MIN + 1, -(int64_t)NSEC_PER_SEC - 1, 0, 0, UNCORRECTABLE),
	DECL_INVALID_TS_TEST(SYS_TIME_T_MIN + 1, -(int64_t)NSEC_PER_SEC - 1, 0, 0, UNCORRECTABLE),
	DECL_INVALID_TS_TEST(SYS_TIME_T_MIN, -1, 0, 0, UNCORRECTABLE),
	DECL_INVALID_TS_TEST(SYS_TIME_T_MAX, (int64_t)NSEC_PER_SEC, 0, 0, UNCORRECTABLE),
	DECL_INVALID_TS_TEST(SYS_TIME_T_MAX - 1, 2 * (int64_t)NSEC_PER_SEC, 0, 0, UNCORRECTABLE),
};

ZTEST(timeutil_api, test_timespec_is_valid)
{
	ARRAY_FOR_EACH(ts_tests, i) {
		const struct ts_test_spec *const tspec = &ts_tests[i];
		bool valid = timespec_is_valid(&tspec->invalid_ts);

		zexpect_equal(valid, tspec->expect_valid,
			      "%d: timespec_is_valid({%ld, %ld}) = %s, expected true", i,
			      tspec->valid_ts.tv_sec, tspec->valid_ts.tv_nsec,
			      tspec->expect_valid ? "false" : "true");
	}
}

ZTEST(timeutil_api, test_timespec_normalize)
{
	ARRAY_FOR_EACH(ts_tests, i) {
		bool different, corrected;
		bool overflow;
		const struct ts_test_spec *const tspec = &ts_tests[i];
		struct timespec norm = tspec->invalid_ts;

		TC_PRINT("%zu: timespec_normalize({%lld, %lld})\n", i,
			 (long long)tspec->invalid_ts.tv_sec, (long long)tspec->invalid_ts.tv_nsec);

		overflow = !timespec_normalize(&norm);
		zexpect_not_equal(tspec->expect_valid || tspec->correctable, overflow,
				  "%d: timespec_normalize({%lld, %lld}) %s, unexpectedly", i,
				  (long long)tspec->invalid_ts.tv_sec,
				  (long long)tspec->invalid_ts.tv_nsec,
				  tspec->correctable ? "failed" : "succeeded");

		if (!tspec->expect_valid && tspec->correctable) {
			different = !timespec_equal(&tspec->invalid_ts, &norm);
			corrected = timespec_equal(&tspec->valid_ts, &norm);
			zexpect_true(different && corrected,
				     "%d: {%lld, %lld} is not properly corrected:"
				     "{%lld, %lld} != {%lld, %lld}", i,
				     (long long)tspec->invalid_ts.tv_sec,
				     (long long)tspec->invalid_ts.tv_nsec,
				     (long long)tspec->valid_ts.tv_sec,
				     (long long)tspec->valid_ts.tv_nsec,
				     (long long)norm.tv_sec,
				     (long long)norm.tv_nsec);
		}
	}
}

ZTEST(timeutil_api, test_timespec_add)
{
	bool overflow;
	struct timespec actual;
	const struct atspec {
		struct timespec a;
		struct timespec b;
		struct timespec result;
		bool expect;
	} tspecs[] = {
		/* non-overflow cases */
		{.a = {0, 0}, .b = {0, 0}, .result = {0, 0}, .expect = false},
		{.a = {1, 1}, .b = {1, 1}, .result = {2, 2}, .expect = false},
		{.a = {-1, 1}, .b = {-1, 1}, .result = {-2, 2}, .expect = false},
		{.a = {-1, NSEC_PER_SEC - 1}, .b = {0, 1}, .result = {0, 0}, .expect = false},
		/* overflow cases */
		{.a = {SYS_TIME_T_MAX, 0}, .b = {1, 0}, .result = {0}, .expect = true},
		{.a = {SYS_TIME_T_MIN, 0}, .b = {-1, 0}, .result = {0}, .expect = true},
		{.a = {SYS_TIME_T_MAX, NSEC_PER_SEC - 1}, .b = {1, 1}, .result = {0},
		 .expect = true},
		{.a = {SYS_TIME_T_MIN, NSEC_PER_SEC - 1}, .b = {-1, 0}, .result = {0},
		 .expect = true},
	};

	ARRAY_FOR_EACH(tspecs, i) {
		const struct atspec *const tspec = &tspecs[i];

		actual = tspec->a;
		overflow = !timespec_add(&actual, &tspec->b);

		zexpect_equal(overflow, tspec->expect,
			      "%d: timespec_add({%ld, %ld}, {%ld, %ld}) %s, unexpectedly", i,
			      tspec->a.tv_sec, tspec->a.tv_nsec, tspec->b.tv_sec, tspec->b.tv_nsec,
			      tspec->expect ? "succeeded" : "failed");

		if (!tspec->expect) {
			zexpect_equal(timespec_equal(&actual, &tspec->result), true,
				      "%d: {%ld, %ld} and {%ld, %ld} are unexpectedly different", i,
				      actual.tv_sec, actual.tv_nsec, tspec->result.tv_sec,
				      tspec->result.tv_nsec);
		}
	}
}

ZTEST(timeutil_api, test_timespec_negate)
{
	struct ntspec {
		struct timespec ts;
		struct timespec result;
		bool expect_failure;
	} tspecs[] = {
		/* non-overflow cases */
		{.ts = {0, 0}, .result = {0, 0}, .expect_failure = false},
		{.ts = {1, 1}, .result = {-2, NSEC_PER_SEC - 1}, .expect_failure = false},
		{.ts = {-1, 1}, .result = {0, NSEC_PER_SEC - 1}, .expect_failure = false},
		{.ts = {SYS_TIME_T_MAX, 0}, .result = {SYS_TIME_T_MIN + 1, 0},
		 .expect_failure = false},
		/* overflow cases */
		{.ts = {SYS_TIME_T_MIN, 0}, .result = {0}, .expect_failure = true},
	};

	ARRAY_FOR_EACH(tspecs, i) {
		bool overflow;
		const struct ntspec *const tspec = &tspecs[i];
		struct timespec actual = tspec->ts;

		overflow = !timespec_negate(&actual);
		zexpect_equal(overflow, tspec->expect_failure,
			      "%d: timespec_negate({%ld, %ld}) %s, unexpectedly", i,
			      tspec->ts.tv_sec, tspec->ts.tv_nsec,
			      tspec->expect_failure ? "did not overflow" : "overflowed");

		if (!tspec->expect_failure) {
			zexpect_true(timespec_equal(&actual, &tspec->result),
				     "%d: {%ld, %ld} and {%ld, %ld} are unexpectedly different", i,
				     actual.tv_sec, actual.tv_nsec, tspec->result.tv_sec,
				     tspec->result.tv_nsec);
		}
	}
}

ZTEST(timeutil_api, test_timespec_sub)
{
	struct timespec a = {0};
	struct timespec b = {0};

	zexpect_true(timespec_sub(&a, &b));
}

ZTEST(timeutil_api, test_timespec_compare)
{
	struct timespec a;
	struct timespec b;

	a = (struct timespec){0};
	b = (struct timespec){0};
	zexpect_equal(timespec_compare(&a, &b), 0);

	a = (struct timespec){-1};
	b = (struct timespec){0};
	zexpect_equal(timespec_compare(&a, &b), -1);

	a = (struct timespec){1};
	b = (struct timespec){0};
	zexpect_equal(timespec_compare(&a, &b), 1);
}

ZTEST(timeutil_api, test_timespec_equal)
{
	struct timespec a;
	struct timespec b;

	a = (struct timespec){0};
	b = (struct timespec){0};
	zexpect_true(timespec_equal(&a, &b));

	a = (struct timespec){-1};
	b = (struct timespec){0};
	zexpect_false(timespec_equal(&a, &b));
}

ZTEST(timeutil_api, test_SYS_TICKS_TO_SECS)
{
	zexpect_equal(SYS_TICKS_TO_SECS(0), 0);
	zexpect_equal(SYS_TICKS_TO_SECS(CONFIG_SYS_CLOCK_TICKS_PER_SEC), 1);
	zexpect_equal(SYS_TICKS_TO_SECS(2 * CONFIG_SYS_CLOCK_TICKS_PER_SEC), 2);
	zexpect_equal(SYS_TICKS_TO_SECS(K_TICKS_FOREVER), SYS_TIME_T_MAX);

	if (SYS_TIME_T_MAX >= 92233720368547758LL) {
		/* These checks should only be done if time_t has enough bits to hold K_TS_MAX */
		zexpect_equal(SYS_TICKS_TO_SECS(K_TICK_MAX), SYS_TIMESPEC_MAX.tv_sec);
#if defined(CONFIG_TIMEOUT_64BIT) && (CONFIG_SYS_CLOCK_TICKS_PER_SEC == 100)
		zexpect_equal(SYS_TIMESPEC_MAX.tv_sec, 92233720368547758LL);
#endif
	}

#if (CONFIG_SYS_CLOCK_TICKS_PER_SEC == 32768)
#if defined(CONFIG_TIMEOUT_64BIT)
	if (SYS_TIME_T_MAX >= 281474976710655LL) {
		zexpect_equal(SYS_TIMESPEC_MAX.tv_sec, 281474976710655LL);
	}
#else
	zexpect_equal(SYS_TIMESPEC_MAX.tv_sec, 131071);
#endif
#endif
}

ZTEST(timeutil_api, test_SYS_TICKS_TO_NSECS)
{
	zexpect_equal(SYS_TICKS_TO_NSECS(0), 0);
	zexpect_equal(SYS_TICKS_TO_NSECS(1) % NSEC_PER_SEC,
		      (NSEC_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC) % NSEC_PER_SEC);
	zexpect_equal(SYS_TICKS_TO_NSECS(2) % NSEC_PER_SEC,
		      (2 * NSEC_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC) % NSEC_PER_SEC);
	zexpect_equal(SYS_TICKS_TO_NSECS(K_TICK_MAX), SYS_TIMESPEC_MAX.tv_nsec);
	zexpect_equal(SYS_TICKS_TO_NSECS(K_TICKS_FOREVER), NSEC_PER_SEC - 1);

#if defined(CONFIG_TIMEOUT_64BIT) && (CONFIG_SYS_CLOCK_TICKS_PER_SEC == 100)
	zexpect_equal(SYS_TIMESPEC_MAX.tv_nsec, 70000000L);
#endif

#if (CONFIG_SYS_CLOCK_TICKS_PER_SEC == 32768)
#if defined(CONFIG_TIMEOUT_64BIT)
	zexpect_equal(SYS_TIMESPEC_MAX.tv_nsec, 999969482L);
#else
	zexpect_equal(SYS_TIMESPEC_MAX.tv_nsec, 999938964L);
#endif
#endif
}

/* non-saturating */
#define DECL_TOSPEC_TEST(to, ts, sat, neg, round)                                                  \
	{                                                                                          \
		.timeout = (to),                                                                   \
		.tspec = (ts),                                                                     \
		.saturation = (sat),                                                               \
		.negative = (neg),                                                                 \
		.roundup = (round),                                                                \
	}
/* negative timespecs rounded up to K_NO_WAIT */
#define DECL_TOSPEC_NEGATIVE_TEST(ts)  DECL_TOSPEC_TEST(K_NO_WAIT, (ts), 0, true, false)
/* zero-valued timeout */
#define DECL_TOSPEC_ZERO_TEST(to)      DECL_TOSPEC_TEST((to), SYS_TIMESPEC(0, 0), 0, false, false)
/* round up toward K_TICK_MIN */
#define DECL_NSAT_TOSPEC_TEST(ts)      DECL_TOSPEC_TEST(K_TICKS(K_TICK_MIN), (ts), -1, false, false)
/* round up toward next tick boundary */
#define DECL_ROUND_TOSPEC_TEST(to, ts) DECL_TOSPEC_TEST((to), (ts), 0, false, true)
/* round down toward K_TICK_MAX */
#define DECL_PSAT_TOSPEC_TEST(ts)      DECL_TOSPEC_TEST(K_TICKS(K_TICK_MAX), (ts), 1, false, false)

static const struct tospec {
	k_timeout_t timeout;
	struct timespec tspec;
	int saturation;
	bool negative;
	bool roundup;
} tospecs[] = {
	/* negative timespecs should round-up to K_NO_WAIT */
	DECL_TOSPEC_NEGATIVE_TEST(SYS_TIMESPEC(SYS_TIME_T_MIN, 0)),
	DECL_TOSPEC_NEGATIVE_TEST(SYS_TIMESPEC(-1, 0)),
	DECL_TOSPEC_NEGATIVE_TEST(SYS_TIMESPEC(-1, NSEC_PER_SEC - 1)),

	/* zero-valued timeouts are equivalent to K_NO_WAIT */
	DECL_TOSPEC_ZERO_TEST(K_NSEC(0)),
	DECL_TOSPEC_ZERO_TEST(K_USEC(0)),
	DECL_TOSPEC_ZERO_TEST(K_MSEC(0)),
	DECL_TOSPEC_ZERO_TEST(K_SECONDS(0)),

	/* round up to K_TICK_MIN */
	DECL_NSAT_TOSPEC_TEST(SYS_TIMESPEC(0, 1)),
	DECL_NSAT_TOSPEC_TEST(SYS_TIMESPEC(0, 2)),
#if CONFIG_SYS_CLOCK_TICKS_PER_SEC > 1
	DECL_NSAT_TOSPEC_TEST(SYS_TIMESPEC(0, SYS_TICKS_TO_NSECS(K_TICK_MIN))),
#endif

#if CONFIG_SYS_CLOCK_TICKS_PER_SEC < MHZ(1)
	DECL_NSAT_TOSPEC_TEST(SYS_TIMESPEC(0, NSEC_PER_USEC)),
#endif
#if CONFIG_SYS_CLOCK_TICKS_PER_SEC < KHZ(1)
	DECL_NSAT_TOSPEC_TEST(SYS_TIMESPEC(0, NSEC_PER_MSEC)),
#endif

/* round to next tick boundary (low-end) */
#if CONFIG_SYS_CLOCK_TICKS_PER_SEC > 1
	DECL_ROUND_TOSPEC_TEST(K_TICKS(2), SYS_TICKS_TO_TIMESPEC_PLUS_NSECS(1, 1)),
	DECL_ROUND_TOSPEC_TEST(K_TICKS(2),
			       SYS_TICKS_TO_TIMESPEC_PLUS_NSECS(1, SYS_TICKS_TO_NSECS(1) / 2)),
	DECL_ROUND_TOSPEC_TEST(K_TICKS(2),
			       SYS_TICKS_TO_TIMESPEC_PLUS_NSECS(1, SYS_TICKS_TO_NSECS(1) - 1)),
#endif

/* exact conversions for large timeouts */
#ifdef CONFIG_TIMEOUT_64BIT
	DECL_TOSPEC_TEST(K_NSEC(2000000000), SYS_TIMESPEC(2, 0), 0, false, false),
#endif
	DECL_TOSPEC_TEST(K_USEC(2000000), SYS_TIMESPEC(2, 0), 0, false, false),
	DECL_TOSPEC_TEST(K_MSEC(2000), SYS_TIMESPEC(2, 0), 0, false, false),

	DECL_TOSPEC_TEST(K_SECONDS(1),
			 SYS_TIMESPEC(1, SYS_TICKS_TO_NSECS(CONFIG_SYS_CLOCK_TICKS_PER_SEC)), 0,
			 false, false),
	DECL_TOSPEC_TEST(K_SECONDS(2),
			 SYS_TIMESPEC(2, SYS_TICKS_TO_NSECS(2 * CONFIG_SYS_CLOCK_TICKS_PER_SEC)), 0,
			 false, false),
	DECL_TOSPEC_TEST(K_SECONDS(100),
			 SYS_TIMESPEC(100,
				      SYS_TICKS_TO_NSECS(100 * CONFIG_SYS_CLOCK_TICKS_PER_SEC)),
			 0, false, false),

	DECL_TOSPEC_TEST(K_TICKS(1000), SYS_TICKS_TO_TIMESPEC(1000), 0, false, false),

/* round to next tick boundary (high-end) */
#if CONFIG_SYS_CLOCK_TICKS_PER_SEC > 1
	DECL_ROUND_TOSPEC_TEST(K_TICKS(1000), SYS_TICKS_TO_TIMESPEC_PLUS_NSECS(999, 1)),
	DECL_ROUND_TOSPEC_TEST(K_TICKS(1000),
			       SYS_TICKS_TO_TIMESPEC_PLUS_NSECS(999, SYS_TICKS_TO_NSECS(1) / 2)),
	DECL_ROUND_TOSPEC_TEST(K_TICKS(1000),
			       SYS_TICKS_TO_TIMESPEC_PLUS_NSECS(999, SYS_TICKS_TO_NSECS(1) - 1)),
#endif

	/* round down toward K_TICK_MAX */
	DECL_PSAT_TOSPEC_TEST(SYS_TICKS_TO_TIMESPEC(K_TICK_MAX)),

	/* K_FOREVER <=> SYS_TIMESPEC_FOREVER */
	DECL_TOSPEC_TEST(K_FOREVER, SYS_TIMESPEC(SYS_TIME_T_MAX, NSEC_PER_SEC - 1), 0, false,
			 false),
};

ZTEST(timeutil_api, test_timespec_from_timeout)
{
	ARRAY_FOR_EACH(tospecs, i) {
		const struct tospec *const tspec = &tospecs[i];
		struct timespec actual;

		/*
		 * In this test we only check exact conversions, so skip negative timespecs that
		 * saturate up to K_NO_WAIT and skip values under SYS_TIMESPEC_MIN and over
		 * SYS_TIMESPEC_MAX. Also, skip "normal" conversions that just round up to the next
		 * tick boundary.
		 */
		if (tspec->negative || (tspec->saturation != 0) || tspec->roundup) {
			continue;
		}

		TC_PRINT("%zu: ticks: {%lld}, timespec: {%lld, %lld}\n", i,
			 (long long)tspec->timeout.ticks, (long long)tspec->tspec.tv_sec,
			 (long long)tspec->tspec.tv_nsec);

		timespec_from_timeout(tspec->timeout, &actual);
		zexpect_true(timespec_equal(&actual, &tspec->tspec),
			     "%d: {%ld, %ld} and {%ld, %ld} are unexpectedly different", i,
			     actual.tv_sec, actual.tv_nsec, tspec->tspec.tv_sec,
			     tspec->tspec.tv_nsec);
	}
}

ZTEST(timeutil_api, test_timespec_to_timeout)
{
	ARRAY_FOR_EACH(tospecs, i) {
		const struct tospec *const tspec = &tospecs[i];
		k_timeout_t actual;
		struct timespec tick_ts;
		struct timespec rem = {};

		TC_PRINT("%zu: ticks: {%lld}, timespec: {%lld, %lld}\n", i,
			 (long long)tspec->timeout.ticks, (long long)tspec->tspec.tv_sec,
			 (long long)tspec->tspec.tv_nsec);

		actual = timespec_to_timeout(&tspec->tspec, &rem);
		if (tspec->saturation == 0) {
			/* exact match or rounding up */
			if (!tspec->negative &&
			    (timespec_compare(&tspec->tspec, &SYS_TIMESPEC_NO_WAIT) != 0) &&
			    (timespec_compare(&tspec->tspec, &SYS_TIMESPEC_FOREVER) != 0)) {
				__ASSERT(timespec_compare(&tspec->tspec, &SYS_TIMESPEC_MIN) >= 0,
					 "%zu: timespec: {%lld, %lld} is not greater than "
					 "SYS_TIMESPEC_MIN",
					 i, (long long)tspec->tspec.tv_sec,
					 (long long)tspec->tspec.tv_nsec);
				__ASSERT(timespec_compare(&tspec->tspec, &SYS_TIMESPEC_MAX) <= 0,
					 "%zu: timespec: {%lld, %lld} is not less than "
					 "SYS_TIMESPEC_MAX",
					 i, (long long)tspec->tspec.tv_sec,
					 (long long)tspec->tspec.tv_nsec);
			}
			zexpect_equal(actual.ticks, tspec->timeout.ticks,
				      "%d: {%" PRId64 "} and {%" PRId64
				      "} are unexpectedly different",
				      i, (int64_t)actual.ticks, (int64_t)tspec->timeout.ticks);
		} else if (tspec->saturation < 0) {
			/* K_TICK_MIN saturation */
			__ASSERT(timespec_compare(&tspec->tspec, &SYS_TIMESPEC_MIN) <= 0,
				 "timespec: {%lld, %lld} is not less than or equal to "
				 "SYS_TIMESPEC_MIN "
				 "{%lld, %lld}",
				 (long long)tspec->tspec.tv_sec, (long long)tspec->tspec.tv_nsec,
				 (long long)SYS_TIMESPEC_MIN.tv_sec,
				 (long long)SYS_TIMESPEC_MIN.tv_nsec);
			zexpect_equal(actual.ticks, K_TICK_MIN,
				      "%d: {%" PRId64 "} and {%" PRId64
				      "} are unexpectedly different",
				      i, (int64_t)actual.ticks, (int64_t)K_TICK_MIN);
		} else if (tspec->saturation > 0) {
			/* K_TICK_MAX saturation */
			__ASSERT(timespec_compare(&tspec->tspec, &SYS_TIMESPEC_MAX) >= 0,
				 "timespec: {%lld, %lld} is not greater than or equal to "
				 "SYS_TIMESPEC_MAX "
				 "{%lld, %lld}",
				 (long long)tspec->tspec.tv_sec, (long long)tspec->tspec.tv_nsec,
				 (long long)SYS_TIMESPEC_MAX.tv_sec,
				 (long long)SYS_TIMESPEC_MAX.tv_nsec);
			zexpect_equal(actual.ticks, K_TICK_MAX,
				      "%d: {%" PRId64 "} and {%" PRId64
				      "} are unexpectedly different",
				      i, (int64_t)actual.ticks, (int64_t)K_TICK_MAX);
		}

		timespec_from_timeout(tspec->timeout, &tick_ts);
		timespec_add(&tick_ts, &rem);
		zexpect_true(timespec_equal(&tick_ts, &tspec->tspec),
			     "%d: {%ld, %ld} and {%ld, %ld} are unexpectedly different", i,
			     tick_ts.tv_sec, tick_ts.tv_nsec, tspec->tspec.tv_sec,
			     tspec->tspec.tv_nsec);
	}

#if defined(CONFIG_TIMEOUT_64BIT) && (CONFIG_SYS_CLOCK_TICKS_PER_SEC == 100)
	{
		struct timespec rem = {};
		k_timeout_t to = K_TICKS(K_TICK_MAX);
		/* SYS_TIMESPEC_MAX corresponding K_TICK_MAX with a tick rate of 100 Hz */
		struct timespec ts = SYS_TIMESPEC(92233720368547758LL, 70000000L);

		zexpect_true(K_TIMEOUT_EQ(timespec_to_timeout(&ts, &rem), to),
			     "timespec_to_timeout(%lld, %lld) != %lld", (long long)ts.tv_sec,
			     (long long)ts.tv_nsec, (long long)to.ticks);
		zexpect_true(timespec_equal(&rem, &SYS_TIMESPEC_NO_WAIT),
			     "non-zero remainder {%lld, %lld}", (long long)rem.tv_sec,
			     (long long)rem.tv_nsec);

		TC_PRINT("timespec_to_timeout():\nts: {%lld, %lld} => to: {%lld}, rem: {%lld, "
			 "%lld}\n",
			 (long long)ts.tv_sec, (long long)ts.tv_nsec, (long long)to.ticks,
			 (long long)rem.tv_sec, (long long)rem.tv_nsec);
	}
#endif
}

static void *setup(void)
{
	TC_PRINT("CONFIG_SYS_CLOCK_TICKS_PER_SEC=%d\n", CONFIG_SYS_CLOCK_TICKS_PER_SEC);
	TC_PRINT("CONFIG_TIMEOUT_64BIT=%c\n", IS_ENABLED(CONFIG_TIMEOUT_64BIT) ? 'y' : 'n');
	TC_PRINT("K_TICK_MIN: %lld\n", (long long)K_TICK_MIN);
	TC_PRINT("K_TICK_MAX: %lld\n", (long long)K_TICK_MAX);
	TC_PRINT("SYS_TIMESPEC_MIN: {%lld, %lld}\n", (long long)SYS_TIMESPEC_MIN.tv_sec,
		 (long long)SYS_TIMESPEC_MIN.tv_nsec);
	TC_PRINT("SYS_TIMESPEC_MAX: {%lld, %lld}\n", (long long)SYS_TIMESPEC_MAX.tv_sec,
		 (long long)SYS_TIMESPEC_MAX.tv_nsec);
	TC_PRINT("INT64_MIN: %lld\n", (long long)INT64_MIN);
	TC_PRINT("INT64_MAX: %lld\n", (long long)INT64_MAX);
	PRINT_LINE;

	/* check numerical values corresponding to K_TICK_MAX */
	zassert_equal(K_TICK_MAX, IS_ENABLED(CONFIG_TIMEOUT_64BIT) ? INT64_MAX : UINT32_MAX - 1);

	return NULL;
}

ZTEST_SUITE(timeutil_api, NULL, setup, NULL, NULL, NULL);
