/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/ztest.h>

#include "tlv320aic3104_clock.h"

static bool decoded_rate_matches(const tlv320aic3104_clock_solution *sol, uint32_t mclk_hz,
				 uint32_t expected_rate_hz)
{
	uint8_t ncodec_code = sol->r2_ndac_nadc & 0x0F;

	zassert_equal(ncodec_code, (sol->r2_ndac_nadc >> 4) & 0x0F, "NDAC != NADC");

	int64_t fsref_x2;

	if (sol->pll_enabled) {
		uint8_t p_code = sol->r3_pll_enable_q_p & 0x07;
		uint8_t r_code = sol->r11_pll_r & 0x0F;
		uint32_t p = (p_code == 0) ? 8 : p_code;
		uint32_t r = (r_code == 0) ? 16 : r_code;
		uint32_t j = (sol->r4_pll_j >> 2) & 0x3F;
		uint32_t d =
			(((uint32_t)sol->r5_pll_d_msb) << 6) | ((sol->r6_pll_d_lsb >> 2) & 0x3F);
		int64_t k10000 = (int64_t)j * 10000 + d;

		int64_t num = (int64_t)mclk_hz * k10000 * r;
		int64_t den = 1024LL * p * 10000LL;

		zassert_equal(num % den, 0, "PLL solution does not reproduce an exact fS(ref)");
		fsref_x2 = num / den;
	} else {
		uint8_t q_code = (sol->r3_pll_enable_q_p >> 3) & 0x0F;
		uint32_t q = (q_code == 0) ? 16 : (q_code == 1) ? 17 : q_code;

		zassert_equal(sol->r3_pll_enable_q_p & 0x80, 0,
			      "divider solution left PLL enabled");

		zassert_equal(mclk_hz % (64u * q), 0, "divider solution does not divide exactly");
		fsref_x2 = mclk_hz / (64u * q);
	}

	int64_t ncodec_x2 = ncodec_code + 2;

	return (fsref_x2 % ncodec_x2 == 0) && (fsref_x2 / ncodec_x2 == expected_rate_hz);
}

ZTEST(tlv320aic3104_clock_solver, test_divider_exact_multiple_disables_pll)
{
	tlv320aic3104_clock_solution sol;

	zassert_equal(0, tlv320aic3104_clock_solve(12288000, 48000, &sol));
	zassert_false(sol.pll_enabled, "exact division must not engage the PLL");
	zassert_equal(sol.r3_pll_enable_q_p & 0x80, 0, "R3 D7 must be 0 when PLL is off");
	zassert_true(decoded_rate_matches(&sol, 12288000, 48000));
}

ZTEST(tlv320aic3104_clock_solver, test_pll_required_for_standard_rate)
{
	tlv320aic3104_clock_solution sol;

	zassert_equal(0, tlv320aic3104_clock_solve(12000000, 44100, &sol));
	zassert_true(sol.pll_enabled, "12 MHz to 44.1 kHz needs the PLL");
	zassert_equal(sol.r3_pll_enable_q_p & 0x80, 0x80, "R3 D7 must be 1 when PLL is on");
	zassert_true(decoded_rate_matches(&sol, 12000000, 44100));
}

ZTEST(tlv320aic3104_clock_solver, test_mclk_below_pll_input_minimum_rejected)
{
	tlv320aic3104_clock_solution sol;

	zassert_equal(-ENOTSUP, tlv320aic3104_clock_solve(400000, 8000, &sol));
}

ZTEST(tlv320aic3104_clock_solver, test_mclk_above_input_maximum_rejected)
{
	tlv320aic3104_clock_solution sol;

	zassert_equal(-ENOTSUP, tlv320aic3104_clock_solve(60000000, 48000, &sol));
}

ZTEST(tlv320aic3104_clock_solver, test_unreachable_rate_rejected)
{
	tlv320aic3104_clock_solution sol;

	zassert_equal(-ENOTSUP, tlv320aic3104_clock_solve(12000000, 44101, &sol));
}

ZTEST(tlv320aic3104_clock_solver, test_every_standard_rate_at_common_mclk)
{

	static const uint32_t rates[] = {8000,  11025, 12000, 16000, 22050,
					 24000, 32000, 44100, 48000};

	for (size_t i = 0; i < ARRAY_SIZE(rates); i++) {
		tlv320aic3104_clock_solution sol;
		int ret = tlv320aic3104_clock_solve(12000000, rates[i], &sol);

		zassert_equal(0, ret, "rate %u should be reachable at 12 MHz", rates[i]);
		zassert_true(decoded_rate_matches(&sol, 12000000, rates[i]),
			     "rate %u decoded incorrectly", rates[i]);
	}
}

ZTEST(tlv320aic3104_clock_solver, test_zero_rate_rejected_before_arithmetic)
{
	tlv320aic3104_clock_solution sol;

	zassert_equal(-EINVAL, tlv320aic3104_clock_solve(12000000, 0, &sol));
}

ZTEST(tlv320aic3104_clock_solver, test_zero_mclk_rejected_before_arithmetic)
{
	tlv320aic3104_clock_solution sol;

	zassert_equal(-EINVAL, tlv320aic3104_clock_solve(0, 48000, &sol));
}

ZTEST(tlv320aic3104_clock_solver, test_fractional_ncodec_divider_honours_fsref_floor)
{
	tlv320aic3104_clock_solution sol;
	uint8_t ncodec_code;
	int64_t ncodec_x2;
	uint8_t q_code;
	uint32_t q;
	int64_t fsref_x2;

	zassert_equal(0, tlv320aic3104_clock_solve(36864000, 16000, &sol));
	zassert_true(decoded_rate_matches(&sol, 36864000, 16000));

	if (sol.pll_enabled) {
		return;
	}

	ncodec_code = sol.r2_ndac_nadc & 0x0F;
	ncodec_x2 = ncodec_code + 2;

	if ((ncodec_x2 % 2) == 0) {
		return;
	}

	q_code = (sol.r3_pll_enable_q_p >> 3) & 0x0F;
	q = (q_code == 0) ? 16 : (q_code == 1) ? 17 : q_code;
	fsref_x2 = 36864000 / (64u * q);

	zassert_true(fsref_x2 >= 39000 * 2,
		     "half-integer NCODEC divider solution at fS(ref) %lld Hz, below the "
		     "39 kHz floor that mode requires",
		     (long long)(fsref_x2 / 2));
}

ZTEST_SUITE(tlv320aic3104_clock_solver, NULL, NULL, NULL, NULL, NULL);
