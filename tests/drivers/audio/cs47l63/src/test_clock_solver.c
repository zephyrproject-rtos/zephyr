/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file test_clock_solver.c
 * @brief cs47l63_clock_solve(), the pure half of the FLL1 path
 *
 * Expected values are DS1249F2's: FLL output 45 to 50 MHz, exactly 49.152 MHz for SYSCLK in
 * the 48 kHz family (section 4.10.7.4), reference at most 13 MHz (section 4.10.7.3).
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/ztest.h>

#include "cs47l63.h"

/** The reference-divider ceiling, DS1249F2 section 4.10.7.3. */
#define FREF_MAX_HZ 13000000U

/** FFLL = (FREF >> FLL1_REFCLK_DIV) * FLL1_FB_DIV * (N + THETA / LAMBDA). */
static uint64_t decoded_fout(const struct cs47l63_fll_solution *sol, uint32_t fref_hz)
{
	uint64_t fref = (uint64_t)fref_hz >> sol->refclk_div;
	uint64_t num = (uint64_t)sol->n * sol->lambda + sol->theta;

	zassert_not_equal(sol->lambda, 0, "LAMBDA is the divisor of the fractional term");
	zassert_true(fref <= FREF_MAX_HZ, "REFCLK_DIV must bring the reference to 13 MHz or below");

	return fref * sol->fb_div * num / sol->lambda;
}

ZTEST_SUITE(cs47l63_clock_solver, NULL, NULL, NULL, NULL, NULL);

ZTEST(cs47l63_clock_solver, test_nominal_6m144_to_49m152)
{
	struct cs47l63_fll_solution sol;

	zassert_ok(cs47l63_clock_solve(6144000, &sol));

	zassert_equal(sol.refclk_div, 0, "6.144 MHz is already below the 13 MHz input ceiling");
	zassert_equal(decoded_fout(&sol, 6144000), CS47L63_FLL_FOUT_HZ,
		      "the solved terms must reproduce 49.152 MHz exactly");
}

ZTEST(cs47l63_clock_solver, test_integer_references_decode_back_exactly)
{
	/* Each reference divides 49.152 MHz exactly: no fractional term. */
	static const uint32_t k_fref[] = {32768, 3072000, 6144000, 12288000};

	for (size_t i = 0; i < ARRAY_SIZE(k_fref); i++) {
		struct cs47l63_fll_solution sol;

		zassert_ok(cs47l63_clock_solve(k_fref[i], &sol),
			   "%u Hz divides 49.152 MHz exactly and must solve", k_fref[i]);
		zassert_equal(sol.theta, 0, "%u Hz needs no fractional term", k_fref[i]);
		zassert_equal(decoded_fout(&sol, k_fref[i]), CS47L63_FLL_FOUT_HZ,
			      "%u Hz decoded to the wrong output", k_fref[i]);
	}
}

ZTEST(cs47l63_clock_solver, test_fractional_reference_decodes_back_exactly)
{
	/* 11.2896 MHz does not divide 49.152 MHz: the ratio is 640 / 147, N = 4 plus 52 / 147. */
	struct cs47l63_fll_solution sol;

	zassert_ok(cs47l63_clock_solve(11289600, &sol));

	zassert_not_equal(sol.theta, 0, "a non-dividing reference needs a fractional term");
	zassert_equal(sol.n, 4, "the integer part of 640 / 147 is 4");
	zassert_equal(sol.lambda, 147, "the ratio in lowest terms has 147 as its denominator");
	zassert_equal(sol.theta, 52, "the remainder of 640 / 147 is 52");
	zassert_equal(decoded_fout(&sol, 11289600), CS47L63_FLL_FOUT_HZ,
		      "N + THETA / LAMBDA must decode back to 49.152 MHz exactly");
}

ZTEST(cs47l63_clock_solver, test_fields_fit_their_register_widths)
{
	struct cs47l63_fll_solution sol;

	zassert_ok(cs47l63_clock_solve(11289600, &sol));

	zassert_equal((uint32_t)sol.n & ~(CS47L63_FLL1_N_MASK >> CS47L63_FLL1_N_SHIFT), 0,
		      "N must fit the ten bits of FLL1_N");
	zassert_equal((uint32_t)sol.fb_div &
			      ~(CS47L63_FLL1_FB_DIV_MASK >> CS47L63_FLL1_FB_DIV_SHIFT),
		      0, "FB_DIV must fit the ten bits of FLL1_FB_DIV");
	zassert_true(sol.refclk_div <= 3, "REFCLK_DIV selects one of /1 /2 /4 /8");
	zassert_true(sol.hp <= 3, "FLL1_HP is a two-bit field");
	zassert_true(sol.lockdet_thr <= 0xF, "FLL1_LOCKDET_THR is a four-bit field");
}

ZTEST(cs47l63_clock_solver, test_zero_reference_rejected)
{
	struct cs47l63_fll_solution sol;

	zassert_equal(-EINVAL, cs47l63_clock_solve(0, &sol),
		      "a zero reference is a caller error, not an unreachable ratio");
}

ZTEST(cs47l63_clock_solver, test_null_solution_rejected)
{
	zassert_equal(-EINVAL, cs47l63_clock_solve(6144000, NULL));
}

ZTEST(cs47l63_clock_solver, test_reference_above_input_ceiling_rejected)
{
	struct cs47l63_fll_solution sol;

	zassert_true((120000000U >> 3) > FREF_MAX_HZ, "the premise: /8 does not bring it in range");
	zassert_equal(-ENOTSUP, cs47l63_clock_solve(120000000, &sol));
}
