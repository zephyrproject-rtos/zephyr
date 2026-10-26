/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/sys/time_units.h>
#include <zephyr/ztest.h>

/* A literal rate divides by reciprocal, a volatile one with the C operator.
 * Both must agree for every conversion form and rounding.
 */
#define CHECK_RATES(from, to, t)                                                                   \
	do {                                                                                       \
		volatile uint32_t vfrom = (from);                                                  \
		volatile uint32_t vto = (to);                                                      \
                                                                                                   \
		for (int r = 0; r < 3; r++) {                                                      \
			bool up = (r == 1);                                                        \
			bool off = (r == 2);                                                       \
			uint64_t t32 = (t) & 0xffffffffU;                                          \
                                                                                                   \
			zassert_equal(z_tmcvt_gen_64_slow(t, from, to, up, off),                   \
				      z_tmcvt_gen_64_slow(t, vfrom, vto, up, off));                \
			zassert_equal(z_tmcvt_gen_32(t32, from, to, up, off),                      \
				      z_tmcvt_gen_32(t32, vfrom, vto, up, off));                   \
			if (z_tmcvt_use_fast_algo(from, to)) {                                     \
				zassert_equal(z_tmcvt_gen_64_fast(t32, from, to, up, off),         \
					      z_tmcvt_gen_64_fast(t32, vfrom, vto, up, off));      \
			}                                                                          \
			if (z_tmcvt_is_int_div(from, to)) {                                        \
				zassert_equal(z_tmcvt_int_div_64(t, from, to, up, off),            \
					      z_tmcvt_int_div_64(t, vfrom, vto, up, off));         \
				zassert_equal(z_tmcvt_int_div_32(t, from, to, up, off),            \
					      z_tmcvt_int_div_32(t, vfrom, vto, up, off));         \
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
