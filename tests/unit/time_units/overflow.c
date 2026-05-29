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
