/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/sys/time_units.h>
#include <zephyr/ztest.h>

/* (t * to + off) / from, exactly, for the expected results */
static uint64_t ref(uint64_t t, uint64_t from, uint64_t to, bool up, bool near)
{
	uint64_t off = near ? (from / 2U) : (up ? (from - 1U) : 0U);

	return ((t / from) * to) + ((((t % from) * to) + off) / from);
}

/* A conversion is exact as long as t * to + from fits in 64 bits, whatever
 * the path: constant t, run-time t with constant rates, or run-time rates.
 */
#define CHECK_ONE(from, to, t, up, near)                                                           \
	do {                                                                                       \
		volatile uint64_t vt = (t);                                                        \
		volatile uint32_t vfrom = (from);                                                  \
		volatile uint32_t vto = (to);                                                      \
		uint64_t e = ref((t), (from), (to), (up), (near));                                 \
                                                                                                   \
		if ((uint64_t)(t) > ((UINT64_MAX - (from)) / (to))) {                              \
			break;                                                                     \
		}                                                                                  \
		zassert_equal(z_tmcvt_64(t, from, to, true, up, near), e);                         \
		zassert_equal(z_tmcvt_32(t, from, to, true, up, near), (uint32_t)e);               \
		zassert_equal(z_tmcvt_64(vt, from, to, true, up, near), e);                        \
		zassert_equal(z_tmcvt_32(vt, from, to, true, up, near), (uint32_t)e);              \
		zassert_equal(z_tmcvt_64(vt, vfrom, vto, false, up, near), e);                     \
		zassert_equal(z_tmcvt_32(vt, vfrom, vto, false, up, near), (uint32_t)e);           \
	} while (false)

#define CHECK_T(from, to, t)                                                                       \
	do {                                                                                       \
		CHECK_ONE(from, to, t, false, false);                                              \
		CHECK_ONE(from, to, t, true, false);                                               \
		CHECK_ONE(from, to, t, false, true);                                               \
	} while (false)

/* Constant inputs on both sides of the 32-bit and 64-bit limits */
#define CHECK_RATES(from, to)                                                                      \
	do {                                                                                       \
		CHECK_T(from, to, 0U);                                                             \
		CHECK_T(from, to, 1U);                                                             \
		CHECK_T(from, to, 9U);                                                             \
		CHECK_T(from, to, 999U);                                                           \
		CHECK_T(from, to, 32767U);                                                         \
		CHECK_T(from, to, 86399U);                                                         \
		CHECK_T(from, to, 0xfffffff5U);                                                    \
		CHECK_T(from, to, 0xffffffffU);                                                    \
		CHECK_T(from, to, 0x100000000ULL);                                                 \
		CHECK_T(from, to, 0x9ffffffffULL);                                                 \
		CHECK_T(from, to, 0x123456789abcdefULL);                                           \
		CHECK_T(from, to, UINT64_MAX - 1U);                                                \
		CHECK_T(from, to, UINT64_MAX);                                                     \
	} while (false)

ZTEST(time_units, test_z_tmcvt_exact)
{
	CHECK_RATES(32768U, 1000U);
	CHECK_RATES(1000U, 32768U);
	CHECK_RATES(1000000U, 32768U);
	CHECK_RATES(32768U, 1000000000U);
	CHECK_RATES(10000U, 1000U);
	CHECK_RATES(1000U, 10000U);
	CHECK_RATES(1000000U, 10000U);
	CHECK_RATES(19200000U, 1000000U);
	CHECK_RATES(24000000U, 1000000000U);
	CHECK_RATES(1000000000U, 32768U);
	CHECK_RATES(12000000U, 1000U);
	CHECK_RATES(3000000000U, 1000U);
	CHECK_RATES(1000U, 100U);
	CHECK_RATES(1000U, 1000U);
	CHECK_RATES(30000U, 48000U);
	CHECK_RATES(7U, 3U);
}

/* The maximum timeout at 2^22 Hz times 1000001, the target rate divided by
 * its greatest common divisor with the source rate, does not fit in 64 bits.
 */
ZTEST(time_units, test_z_tmcvt_max_timeout_reduced)
{
	uint64_t secs = (uint64_t)CONFIG_SYS_CLOCK_MAX_TIMEOUT_DAYS * 24U * 3600U;
	volatile uint64_t t = secs * 4194304U;

	zassert_equal(z_tmcvt_64(t, 4194304U, 4096004096U, true, false, false),
		      secs * 4096004096U);
}

/* A conversion cast to bool must build without -Wint-in-bool-context. */
ZTEST(time_units, test_z_tmcvt_bool)
{
	volatile uint64_t t = 1U;

	zassert_true((bool)z_tmcvt_32(t, 100U, 1000U, true, false, false));
	zassert_true((bool)z_tmcvt_64(t, 100U, 1000U, true, false, false));
}

/* The result of a 32-bit conversion is 32-bit, even with 64-bit rates. */
BUILD_ASSERT(sizeof(z_tmcvt_32(1U, 3000000000ULL, 1000ULL, true, false, false)) ==
	     sizeof(uint32_t));

/* A constant conversion is a constant expression. */
static const uint32_t static_init = z_tmcvt_32(5U, 32768U, 1000U, true, true, false);

ZTEST(time_units, test_z_tmcvt_static)
{
	zassert_equal(static_init, 1U);
}
