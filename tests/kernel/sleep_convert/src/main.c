/*
 * Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Exercise the tick to millisecond and tick to microsecond converters that
 * k_sleep() and k_usleep() use on their return value.
 *
 * Those converters pick one of several forms at compile time from
 * CONFIG_SYS_CLOCK_TICKS_PER_SEC, and the narrow ones are only safe because
 * of a clamp applied beforehand in the tick domain.  So the interesting
 * coverage is a whole build per tick rate, which tests.yaml provides:
 *
 *   100      ms: whole milliseconds per tick   us: whole microseconds per tick
 *   1000     ms: 1:1                           us: whole microseconds per tick
 *   10000    ms: whole ticks per millisecond   us: whole microseconds per tick
 *   32768    ms: split                         us: two stage split
 *   12345    ms: split                         us: two stage split
 *   1000000  ms: whole ticks per millisecond   us: 1:1
 *
 * The reference is the generic 64 bit converter plus an explicit saturation,
 * which is the contract these helpers must meet.  On a 64 bit target the
 * helpers are defined to be exactly that, so there the checks below only
 * really police the saturation, which is the point they still earn.
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#define TICK_HZ ((uint32_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/* Highest tick count k_sleep_ticks() can report, being the result of a
 * 32 bit subtraction narrowed to a positive int32_t.
 */
#define MAX_SLEEP_TICKS ((uint32_t)INT32_MAX)

static int32_t ref_ms(uint32_t ticks)
{
	uint64_t ms = k_ticks_to_ms_ceil64((uint64_t)ticks);

	return ms > (uint64_t)INT32_MAX ? INT32_MAX : (int32_t)ms;
}

static int32_t ref_us(uint32_t ticks)
{
	uint64_t us = k_ticks_to_us_ceil64((uint64_t)ticks);

	return us > (uint64_t)INT32_MAX ? INT32_MAX : (int32_t)us;
}

static void check(uint32_t ticks)
{
	zassert_equal(z_sleep_ticks_to_int32_ms(ticks), ref_ms(ticks),
		      "ms conversion of %u ticks: got %d, expected %d", ticks,
		      z_sleep_ticks_to_int32_ms(ticks), ref_ms(ticks));

	zassert_equal(z_sleep_ticks_to_int32_us(ticks), ref_us(ticks),
		      "us conversion of %u ticks: got %d, expected %d", ticks,
		      z_sleep_ticks_to_int32_us(ticks), ref_us(ticks));
}

/* Every tick count from zero up, where the quotient and the remainder of the
 * split forms both change on every step.
 */
ZTEST(sleep_convert, test_small_values)
{
	for (uint32_t t = 0; t <= 10000U; t++) {
		check(t);
	}
}

/* A stride that is coprime with the usual tick rates, so it lands on a varied
 * set of remainders all the way up to the top of the range.  Kept sparse on
 * purpose: this is here for reach, while the cases that actually distinguish
 * the conversion forms are covered densely above and exactly below.
 */
ZTEST(sleep_convert, test_whole_range)
{
	for (uint64_t t = 0; t <= (uint64_t)MAX_SLEEP_TICKS; t += 1048573U) {
		check((uint32_t)t);
	}
}

/* Where the converters change behaviour: around the tick rate itself, which
 * is where the quotient and remainder split, and around the clamp bounds.
 */
ZTEST(sleep_convert, test_boundaries)
{
	uint64_t clamp_ms = (uint64_t)INT32_MAX * TICK_HZ / MSEC_PER_SEC;
	uint64_t clamp_us = (uint64_t)INT32_MAX * TICK_HZ / USEC_PER_SEC;
	uint64_t probes[] = {
		0, 1, 2,
		TICK_HZ - 1, TICK_HZ, TICK_HZ + 1,
		(uint64_t)TICK_HZ * 2, (uint64_t)TICK_HZ * 2 + 1,
		clamp_ms - 1, clamp_ms, clamp_ms + 1,
		clamp_us - 1, clamp_us, clamp_us + 1,
		MAX_SLEEP_TICKS - 1, MAX_SLEEP_TICKS,
	};

	for (unsigned int i = 0; i < ARRAY_SIZE(probes); i++) {
		if (probes[i] > (uint64_t)MAX_SLEEP_TICKS) {
			continue;
		}
		check((uint32_t)probes[i]);
	}
}

/* Past the clamp bound the result must saturate rather than wrap, which is
 * the property that lets the conversion itself stay narrow.
 */
ZTEST(sleep_convert, test_saturation)
{
	uint64_t clamp_ms = (uint64_t)INT32_MAX * TICK_HZ / MSEC_PER_SEC;
	uint64_t clamp_us = (uint64_t)INT32_MAX * TICK_HZ / USEC_PER_SEC;

	if (clamp_ms < (uint64_t)MAX_SLEEP_TICKS) {
		zassert_equal(z_sleep_ticks_to_int32_ms(MAX_SLEEP_TICKS), INT32_MAX,
			      "ms conversion failed to saturate");
		zassert_equal(z_sleep_ticks_to_int32_ms((uint32_t)clamp_ms + 1), INT32_MAX,
			      "ms conversion failed to saturate just past the bound");
	} else {
		/* every reachable tick count is representable in milliseconds */
		zassert_true(z_sleep_ticks_to_int32_ms(MAX_SLEEP_TICKS) <= INT32_MAX);
	}

	if (clamp_us < (uint64_t)MAX_SLEEP_TICKS) {
		zassert_equal(z_sleep_ticks_to_int32_us(MAX_SLEEP_TICKS), INT32_MAX,
			      "us conversion failed to saturate");
		zassert_equal(z_sleep_ticks_to_int32_us((uint32_t)clamp_us + 1), INT32_MAX,
			      "us conversion failed to saturate just past the bound");
	} else {
		zassert_true(z_sleep_ticks_to_int32_us(MAX_SLEEP_TICKS) <= INT32_MAX);
	}
}

/* The conversion must never round down: sleeping for the reported remainder
 * has to cover the time that was actually left.
 */
ZTEST(sleep_convert, test_rounds_up)
{
	for (uint32_t t = 1; t <= 2000U; t++) {
		int32_t ms = z_sleep_ticks_to_int32_ms(t);
		int32_t us = z_sleep_ticks_to_int32_us(t);

		zassert_true((uint64_t)k_ms_to_ticks_floor64(ms) >= t || ms == INT32_MAX,
			     "%d ms is short of %u ticks", ms, t);
		zassert_true((uint64_t)k_us_to_ticks_floor64(us) >= t || us == INT32_MAX,
			     "%d us is short of %u ticks", us, t);
	}
}

/* Defined in cpp_build.cpp: exists so the header is compiled as C++ too. */
extern void sleep_convert_cpp_build(void);

ZTEST(sleep_convert, test_cpp_build)
{
	sleep_convert_cpp_build();
}

ZTEST_SUITE(sleep_convert, NULL, NULL, NULL, NULL, NULL);
