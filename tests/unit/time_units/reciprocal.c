/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/sys/time_units.h>
#include <zephyr/ztest.h>

/* The div64 helpers must agree with plain C division for every conversion
 * form and rounding.
 */
#define CHECK_RATES(from, to, t)                                                                   \
	do {                                                                                       \
		for (int r = 0; r < 3; r++) {                                                      \
			bool up = (r == 1);                                                        \
			bool off = (r == 2);                                                       \
			uint64_t t32 = (t) & 0xffffffffU;                                          \
                                                                                                   \
			zassert_equal(z_tmcvt_gen_64_slow(z_tmcvt_r, t, from, to, up, off),        \
				      z_tmcvt_gen_64_slow(z_tmcvt_c, t, from, to, up, off));       \
			zassert_equal(z_tmcvt_gen_32(z_tmcvt_r, t32, from, to, up, off),           \
				      z_tmcvt_gen_32(z_tmcvt_c, t32, from, to, up, off));          \
			if (z_tmcvt_use_fast_algo(from, to)) {                                     \
				zassert_equal(                                                     \
					z_tmcvt_gen_64_fast(z_tmcvt_r, t32, from, to, up, off),    \
					z_tmcvt_gen_64_fast(z_tmcvt_c, t32, from, to, up, off));   \
			}                                                                          \
			if (z_tmcvt_is_int_div(from, to)) {                                        \
				zassert_equal(                                                     \
					z_tmcvt_int_div_64(z_tmcvt_r, t, from, to, up, off),       \
					z_tmcvt_int_div_64(z_tmcvt_c, t, from, to, up, off));      \
				zassert_equal(                                                     \
					z_tmcvt_int_div_32(z_tmcvt_r, t, from, to, up, off),       \
					z_tmcvt_int_div_32(z_tmcvt_c, t, from, to, up, off));      \
			}                                                                          \
		}                                                                                  \
	} while (false)

ZTEST(time_units, test_z_tmcvt_reciprocal)
{
	static const uint64_t values[] = {
		0U, 1U, 999U, 1000U, 32767U, 32768U, 86399U, 0xffffffffU, 0x100000000ULL,
		0x123456789abcdefULL, 0xfedcba9876543210ULL, UINT64_MAX - 1U, UINT64_MAX,
	};

	ARRAY_FOR_EACH(values, i) {
		uint64_t t = values[i];

		CHECK_RATES(32768U, 1000U, t);
		CHECK_RATES(1000U, 32768U, t);
		CHECK_RATES(10000U, 1000U, t);
		CHECK_RATES(1000000U, 10000U, t);
		CHECK_RATES(19200000U, 1000000U, t);
		CHECK_RATES(24000000U, 1000000000U, t);
		CHECK_RATES(1000000000U, 32768U, t);
		CHECK_RATES(12000000U, 1000U, t);
		CHECK_RATES(1000U, 100U, t);
	}
}

/* A conversion cast to bool must build without -Wint-in-bool-context. */
ZTEST(time_units, test_z_tmcvt_bool)
{
	volatile uint64_t t = 1U;

	zassert_true((bool)z_tmcvt_32(t, 100U, 1000U, true, false, false));
	zassert_true((bool)z_tmcvt_64(t, 100U, 1000U, true, false, false));
}

/* The 32-bit integer division path must give the exact result on both sides
 * of the input range where it divides in 32 bits, and a 32-bit result even
 * when the rates are 64-bit constants.
 */
ZTEST(time_units, test_z_tmcvt_int_div_32)
{
	static const uint64_t values[] = {
		0U,          1U,          9U,          10U,         0xfffffff5U,
		0xfffffff6U, 0xfffffff7U, 0xffffffffU, 0x100000000ULL, 0x9ffffffffULL,
	};

	BUILD_ASSERT(sizeof(z_tmcvt_int_div_32(z_tmcvt_c, 1U, 3000000000ULL, 1000ULL, false,
					       false)) == sizeof(uint32_t));

	ARRAY_FOR_EACH(values, i) {
		uint64_t t = values[i];

		zassert_equal(z_tmcvt_int_div_32(z_tmcvt_r, t, 10000U, 1000U, true, false),
			      (uint32_t)((t + 9U) / 10U));
		zassert_equal(z_tmcvt_int_div_32(z_tmcvt_r, t, 10000U, 1000U, false, false),
			      (uint32_t)(t / 10U));
		zassert_equal(z_tmcvt_int_div_32(z_tmcvt_r, t, 3000000000ULL, 1000ULL, true, false),
			      (uint32_t)((t + 2999999U) / 3000000U));
	}
}
