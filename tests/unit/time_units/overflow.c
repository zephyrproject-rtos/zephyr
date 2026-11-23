/*
 * Copyright 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/sys/time_units.h>
#include <zephyr/ztest.h>

/**
 * @brief Test @ref z_tmcvt for robustness against intermediate value overflow.
 *
 * With input
 * ```
 * [t0, t1, t2] = [
 *   UINT64_MAX / to_hz - 1,
 *   UINT64_MAX / to_hz,
 *   UINT64_MAX / to_hz + 1,
 * ]
 * ```
 *
 * passed through @ref z_tmcvt, we expect a linear sequence:
 * ```
 * [
 *   562949953369140,
 *   562949953399658,
 *   562949953430175,
 * ]
 * ```
 *
 * If an overflow occurs, we see something like the following:
 * ```
 * [
 *   562949953369140,
 *   562949953399658,
 *   8863,
 * ]
 * ```
 */
ZTEST(time_units, test_z_tmcvt_for_overflow)
{
	const uint32_t from_hz = 32768UL;
	const uint32_t to_hz = 1000000000UL;

	zassert_equal(562949953369140ULL,
		      z_tmcvt(UINT64_MAX / to_hz - 1, from_hz, to_hz, true, false, false, false));
	zassert_equal(562949953399658ULL,
		      z_tmcvt(UINT64_MAX / to_hz, from_hz, to_hz, true, false, false, false));
	zassert_equal(562949953430175ULL,
		      z_tmcvt(UINT64_MAX / to_hz + 1, from_hz, to_hz, true, false, false, false));
}

ZTEST(time_units, test_z_tmcvt_int_mul_32_overflow)
{
	/*
	 * Ensure signed int inputs whose scaled 32-bit result exceeds INT32_MAX
	 * (yet fits in uint32_t, or wraps modulo 2^32) convert in unsigned
	 * 32-bit arithmetic without signed integer overflow.
	 */
	zassert_equal(3000000000U, z_tmcvt(300000000, 1000, 10000, true, true, true, false));
	zassert_equal((uint32_t)((uint32_t)INT32_MAX * 10U),
		      z_tmcvt(INT32_MAX, 1000, 10000, true, true, true, false));
}
