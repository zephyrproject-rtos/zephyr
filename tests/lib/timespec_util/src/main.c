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

BUILD_ASSERT(sizeof(time_t) == sizeof(int64_t), "time_t must be 64-bit");
BUILD_ASSERT(sizeof(((struct timespec *)0)->tv_sec) == sizeof(int64_t), "tv_sec must be 64-bit");

/* need NSEC_PER_SEC to be signed for the purposes of this testsuite */
#undef NSEC_PER_SEC
#define NSEC_PER_SEC 1000000000L

#undef CORRECTABLE
#define CORRECTABLE true

#undef UNCORRECTABLE
#define UNCORRECTABLE false

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
	DECL_VALID_TS_TEST(INT64_MIN, 0),
	DECL_VALID_TS_TEST(INT64_MIN, NSEC_PER_SEC - 1),
	DECL_VALID_TS_TEST(INT64_MAX, 0),
	DECL_VALID_TS_TEST(INT64_MAX, NSEC_PER_SEC - 1),

	/* Correctable, invalid cases */
	DECL_INVALID_TS_TEST(0, -2 * NSEC_PER_SEC + 1, -2, 1, CORRECTABLE),
	DECL_INVALID_TS_TEST(0, -2 * NSEC_PER_SEC - 1, -3, NSEC_PER_SEC - 1, CORRECTABLE),
	DECL_INVALID_TS_TEST(0, -NSEC_PER_SEC - 1, -2, NSEC_PER_SEC - 1, CORRECTABLE),
	DECL_INVALID_TS_TEST(0, -1, -1, NSEC_PER_SEC - 1, CORRECTABLE),
	DECL_INVALID_TS_TEST(0, NSEC_PER_SEC, 1, 0, CORRECTABLE),
	DECL_INVALID_TS_TEST(0, NSEC_PER_SEC + 1, 1, 0, CORRECTABLE),
	DECL_INVALID_TS_TEST(1, -1, 0, NSEC_PER_SEC - 1, CORRECTABLE),
	DECL_INVALID_TS_TEST(1, NSEC_PER_SEC, 2, 0, CORRECTABLE),
	DECL_INVALID_TS_TEST(-1, -1, -2, NSEC_PER_SEC - 1, CORRECTABLE),
	DECL_INVALID_TS_TEST(0, NSEC_PER_SEC, 1, 0, CORRECTABLE),
	DECL_INVALID_TS_TEST(1, -1, 0, NSEC_PER_SEC - 1, CORRECTABLE),
	DECL_INVALID_TS_TEST(1, NSEC_PER_SEC, 2, 0, CORRECTABLE),
	DECL_INVALID_TS_TEST(INT64_MIN, NSEC_PER_SEC, INT64_MIN + 1, 0, CORRECTABLE),
	DECL_INVALID_TS_TEST(INT64_MAX, -1, INT64_MAX - 1, NSEC_PER_SEC - 1, CORRECTABLE),
	DECL_INVALID_TS_TEST(0, LONG_MIN, LONG_MAX / NSEC_PER_SEC, 145224192, CORRECTABLE),
	DECL_INVALID_TS_TEST(0, LONG_MAX, LONG_MAX / NSEC_PER_SEC, LONG_MAX % NSEC_PER_SEC,
			     CORRECTABLE),

	/* Uncorrectable, invalid cases */
	DECL_INVALID_TS_TEST(INT64_MIN + 2, -2 * NSEC_PER_SEC - 1, 0, 0, UNCORRECTABLE),
	DECL_INVALID_TS_TEST(INT64_MIN + 1, -NSEC_PER_SEC - 1, 0, 0, UNCORRECTABLE),
	DECL_INVALID_TS_TEST(INT64_MIN + 1, -NSEC_PER_SEC - 1, 0, 0, UNCORRECTABLE),
	DECL_INVALID_TS_TEST(INT64_MIN, -1, 0, 0, UNCORRECTABLE),
	DECL_INVALID_TS_TEST(INT64_MAX, NSEC_PER_SEC, 0, 0, UNCORRECTABLE),
	DECL_INVALID_TS_TEST(INT64_MAX - 1, 2 * NSEC_PER_SEC, 0, 0, UNCORRECTABLE),
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
		bool different;
		bool overflow;
		const struct ts_test_spec *const tspec = &ts_tests[i];
		struct timespec norm = tspec->invalid_ts;

		overflow = !timespec_normalize(&norm);
		zexpect_not_equal(tspec->expect_valid || tspec->correctable, overflow,
				  "%d: timespec_normalize({%ld, %ld}) %s, unexpectedly", i,
				  (long)tspec->invalid_ts.tv_sec, (long)tspec->invalid_ts.tv_nsec,
				  tspec->correctable ? "failed" : "succeeded");

		if (!tspec->expect_valid && tspec->correctable) {
			different = !timespec_equal(&tspec->invalid_ts, &norm);
			zexpect_true(different, "%d: {%ld, %ld} and {%ld, %ld} are unexpectedly %s",
				     i, tspec->invalid_ts.tv_sec, tspec->invalid_ts.tv_nsec,
				     norm.tv_sec, tspec->valid_ts.tv_sec,
				     (tspec->expect_valid || tspec->correctable) ? "different"
										 : "equal");
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
		{.a = {INT64_MAX, 0}, .b = {1, 0}, .result = {0}, .expect = true},
		{.a = {INT64_MIN, 0}, .b = {-1, 0}, .result = {0}, .expect = true},
		{.a = {INT64_MAX, NSEC_PER_SEC - 1}, .b = {1, 1}, .result = {0}, .expect = true},
		{.a = {INT64_MIN, NSEC_PER_SEC - 1}, .b = {-1, 0}, .result = {0}, .expect = true},
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
		{.ts = {INT64_MAX, 0}, .result = {INT64_MIN + 1, 0}, .expect_failure = false},
		/* overflow cases */
		{.ts = {INT64_MIN, 0}, .result = {0}, .expect_failure = true},
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

/* non-saturating */
#define DECL_TOSPEC_TEST(to, sec, nsec, sat, neg)                                                  \
	{                                                                                          \
		.timeout = (to),                                                                   \
		.tspec =                                                                           \
			{                                                                          \
				.tv_sec = (sec),                                                   \
				.tv_nsec = (nsec),                                                 \
			},                                                                         \
		.saturation = (sat),                                                               \
		.negative = (neg),                                                                 \
	}
/* negative timespecs rounded up to K_NO_WAIT */
#define DECL_TOSPEC_NEGATIVE_TEST(sec, nsec) DECL_TOSPEC_TEST(K_NO_WAIT, (sec), (nsec), 0, true)
/* zero-valued timeout (no saturation / exact) */
#define DECL_TOSPEC_ZERO_TEST(to)            DECL_TOSPEC_TEST((to), 0, 0, 0, false)
/* round up toward K_TICK_MIN */
#define DECL_NSAT_TOSPEC_TEST(sec, nsec)                                                           \
	DECL_TOSPEC_TEST(K_TICKS(K_TICK_MIN), (sec), (nsec), -1, false)
/* round down toward K_TICK_MAX */
#define DECL_PSAT_TOSPEC_TEST(sec, nsec)                                                           \
	DECL_TOSPEC_TEST(K_TICKS(K_TICK_MAX), (sec), (nsec), 1, false)

static const struct tospec {
	k_timeout_t timeout;
	struct timespec tspec;
	int saturation;
	bool negative;
} tospecs[] = {
	/* negative timespecs should round-up to K_NO_WAIT */
	DECL_TOSPEC_NEGATIVE_TEST(INT64_MIN, 0),
	DECL_TOSPEC_NEGATIVE_TEST(-1, 0),
	DECL_TOSPEC_NEGATIVE_TEST(-1, NSEC_PER_SEC - 1),

	/* zero-valued timeouts are equivalent to K_NO_WAIT */
	DECL_TOSPEC_ZERO_TEST(K_NSEC(0)),
	DECL_TOSPEC_ZERO_TEST(K_USEC(0)),
	DECL_TOSPEC_ZERO_TEST(K_MSEC(0)),
	DECL_TOSPEC_ZERO_TEST(K_SECONDS(0)),

	/* round up to K_TICK_MIN */
	DECL_NSAT_TOSPEC_TEST(0, 1),
	DECL_NSAT_TOSPEC_TEST(0, 2),
	DECL_NSAT_TOSPEC_TEST(0, (long)(K_TICKS_TO_NSECS(K_TICK_MIN) % NSEC_PER_SEC)),

#if CONFIG_SYS_CLOCK_TICKS_PER_SEC < GHZ(1)
	DECL_TOSPEC_TEST(K_NSEC(1), 0, (long)(K_TICKS_TO_NSECS(K_TICK_MIN) % NSEC_PER_SEC), 0,
			 false),
#endif
#if CONFIG_SYS_CLOCK_TICKS_PER_SEC < MHZ(1)
	DECL_TOSPEC_TEST(K_USEC(1), 0, (long)(K_TICKS_TO_NSECS(K_TICK_MIN) % NSEC_PER_SEC), 0,
			 false),
#endif
#if CONFIG_SYS_CLOCK_TICKS_PER_SEC < KHZ(1)
	DECL_TOSPEC_TEST(K_MSEC(1), 0, (long)(K_TICKS_TO_NSECS(K_TICK_MIN) % NSEC_PER_SEC), 0,
			 false),
#endif

	/* round to next tick boundary */
	DECL_TOSPEC_TEST(K_TICKS(1), 0, K_TICKS_TO_NSECS(1) % NSEC_PER_SEC, 0, false),
	DECL_TOSPEC_TEST(K_TICKS(2), 0, K_TICKS_TO_NSECS(2) % NSEC_PER_SEC, 0, false),

	DECL_TOSPEC_TEST(K_NSEC(2000000000), 2, 0, 0, false),
	DECL_TOSPEC_TEST(K_USEC(2000000), 2, 0, 0, false),
	DECL_TOSPEC_TEST(K_MSEC(2000), 2, 0, 0, false),

	DECL_TOSPEC_TEST(K_SECONDS(1), 1, 0, 0, false),
	DECL_TOSPEC_TEST(K_SECONDS(2), 2, 0, 0, false),
	DECL_TOSPEC_TEST(K_SECONDS(100), 100, 0, 0, false),

	DECL_TOSPEC_TEST(K_TICKS(1000), 1000 / CONFIG_SYS_CLOCK_TICKS_PER_SEC,
			 K_TICKS_TO_NSECS(1000) % NSEC_PER_SEC, 0, false),

	/* round down toward K_TICK_MAX */
	DECL_PSAT_TOSPEC_TEST((uint64_t)K_TICK_MAX / CONFIG_SYS_CLOCK_TICKS_PER_SEC,
			      K_TICKS_TO_NSECS(K_TICK_MAX) % NSEC_PER_SEC),
	DECL_PSAT_TOSPEC_TEST(((uint64_t)K_TICK_MAX + 1) / CONFIG_SYS_CLOCK_TICKS_PER_SEC, 0),
	DECL_PSAT_TOSPEC_TEST(((uint64_t)K_TICK_MAX + 1) / CONFIG_SYS_CLOCK_TICKS_PER_SEC,
			      K_TICKS_TO_NSECS(((uint64_t)K_TICK_MAX + 1)) % NSEC_PER_SEC),

	/* forever-valued timeouts are equivalent to K_FOREVER */
	DECL_TOSPEC_TEST(K_FOREVER, INT64_MAX, NSEC_PER_SEC - 1, 0, false),
};

ZTEST(timeutil_api, test_timespec_from_timeout)
{
	ARRAY_FOR_EACH(tospecs, i) {
		const struct tospec *const tspec = &tospecs[i];
		struct timespec actual;

		/*
		 * In this test we only check exact conversions, so skip negative timespecs that
		 * saturate up to K_NO_WAIT and skip values under K_TS_MIN and over K_TS_MAX
		 */
		if (tspec->negative || (tspec->saturation != 0)) {
			continue;
		}

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
		struct timespec rem;
		struct timespec tick_ts;

		if (tspec->saturation == 0) {
			/* no saturation / exact match */
			actual = timespec_to_timeout_rem(&tspec->tspec, &rem);
			zexpect_equal(actual.ticks, tspec->timeout.ticks,
				      "%d: {%" PRId64 "} and {%" PRId64
				      "} are unexpectedly different",
				      i, (int64_t)actual.ticks, (int64_t)tspec->timeout.ticks);
		} else if (tspec->saturation < 0) {
			/* K_TICK_MIN saturation */
			actual = timespec_to_timeout_rem(&tspec->tspec, &rem);
			zexpect_equal(actual.ticks, K_TICK_MIN,
				      "%d: {%" PRId64 "} and {%" PRId64
				      "} are unexpectedly different",
				      i, (int64_t)actual.ticks, (int64_t)K_TICK_MIN);
		} else if (tspec->saturation > 0) {
			/* K_TICK_MAX saturation */
			actual = timespec_to_timeout_rem(&tspec->tspec, &rem);
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
}

static void *setup(void)
{
	TC_PRINT("CONFIG_SYS_CLOCK_TICKS_PER_SEC=%d\n", CONFIG_SYS_CLOCK_TICKS_PER_SEC);
	TC_PRINT("CONFIG_TIMEOUT_64BIT=%c\n", IS_ENABLED(CONFIG_TIMEOUT_64BIT) ? 'y' : 'n');
	TC_PRINT("K_TICK_MIN: %lld\n", (long long)K_TICK_MIN);
	TC_PRINT("K_TICK_MAX: %lld\n", (long long)K_TICK_MAX);
	TC_PRINT("K_TS_MIN: {%lld, %lld}\n", (long long)K_TS_MIN.tv_sec,
		 (long long)K_TS_MIN.tv_nsec);
	TC_PRINT("K_TS_MAX: {%lld, %lld}\n", (long long)K_TS_MAX.tv_sec,
		 (long long)K_TS_MAX.tv_nsec);
	PRINT_LINE;

	return NULL;
}

ZTEST_SUITE(timeutil_api, NULL, setup, NULL, NULL, NULL);
