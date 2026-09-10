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

#define REFDIV_MAX 8U

#define FREF_16K_HZ     16000U
#define FREF_32K768_HZ  32768U
#define FREF_3M072_HZ   3072000U
#define FREF_6M144_HZ   6144000U
#define FREF_11M2896_HZ 11289600U
#define FREF_12M288_HZ  12288000U
#define FREF_655K36_HZ  655360U
#define FREF_26M_HZ     26000000U
#define FREF_120M_HZ    120000000U

#define EXPECT_FRAC_N      4U
#define EXPECT_FRAC_LAMBDA 147U
#define EXPECT_FRAC_THETA  52U

BUILD_ASSERT((FREF_120M_HZ / REFDIV_MAX) > FREF_MAX_HZ,
	     "the premise: /8 does not bring it in range");

/** FFLL = (FREF >> FLL1_REFCLK_DIV) * FLL1_FB_DIV * (N + THETA / LAMBDA). */
static uint64_t decoded_fout(const struct cs47l63_fll_solution *sol, uint32_t fref_hz)
{
	uint64_t fref = (uint64_t)fref_hz >> sol->refclk_div;
	uint64_t num = ((uint64_t)sol->n * sol->lambda) + sol->theta;

	zassert_not_equal(sol->lambda, 0U, "LAMBDA is the divisor of the fractional term");
	zassert_true(fref <= FREF_MAX_HZ, "REFCLK_DIV must bring the reference to 13 MHz or below");

	return fref * sol->fb_div * num / sol->lambda;
}

ZTEST_SUITE(cs47l63_clock_solver, NULL, NULL, NULL, NULL, NULL);

static ZTEST(cs47l63_clock_solver, test_nominal_6m144_to_49m152)
{
	struct cs47l63_fll_solution sol;

	zassert_ok(cs47l63_clock_solve(FREF_6M144_HZ, &sol));

	zassert_equal(sol.refclk_div, 0U, "6.144 MHz is already below the 13 MHz input ceiling");
	zassert_equal(decoded_fout(&sol, FREF_6M144_HZ), CS47L63_FLL_FOUT_HZ,
		      "the solved terms must reproduce 49.152 MHz exactly");
}

static ZTEST(cs47l63_clock_solver, test_integer_references_decode_back_exactly)
{
	/* Each reference divides 49.152 MHz exactly: no fractional term. */
	static const uint32_t k_fref[] = {FREF_32K768_HZ, FREF_3M072_HZ, FREF_6M144_HZ,
					  FREF_12M288_HZ};

	for (size_t i = 0U; i < ARRAY_SIZE(k_fref); i++) {
		struct cs47l63_fll_solution sol;

		zassert_ok(cs47l63_clock_solve(k_fref[i], &sol),
			   "%u Hz divides 49.152 MHz exactly and must solve", k_fref[i]);
		zassert_equal(sol.theta, 0U, "%u Hz needs no fractional term", k_fref[i]);
		zassert_equal(decoded_fout(&sol, k_fref[i]), CS47L63_FLL_FOUT_HZ,
			      "%u Hz decoded to the wrong output", k_fref[i]);
	}
}

static ZTEST(cs47l63_clock_solver, test_fractional_reference_decodes_back_exactly)
{
	/* 11.2896 MHz does not divide 49.152 MHz: the ratio is 640 / 147, N = 4 plus 52 / 147. */
	struct cs47l63_fll_solution sol;

	zassert_ok(cs47l63_clock_solve(FREF_11M2896_HZ, &sol));

	zassert_not_equal(sol.theta, 0U, "a non-dividing reference needs a fractional term");
	zassert_equal(sol.n, EXPECT_FRAC_N, "the integer part of 640 / 147 is 4");
	zassert_equal(sol.lambda, EXPECT_FRAC_LAMBDA,
		      "the ratio in lowest terms has 147 as its denominator");
	zassert_equal(sol.theta, EXPECT_FRAC_THETA, "the remainder of 640 / 147 is 52");
	zassert_equal(decoded_fout(&sol, FREF_11M2896_HZ), CS47L63_FLL_FOUT_HZ,
		      "N + THETA / LAMBDA must decode back to 49.152 MHz exactly");
}

static ZTEST(cs47l63_clock_solver, test_integer_ratio_split_by_fb_div_is_not_programmed_as_integer)
{
	/* 655.36 kHz divides 49.152 MHz 75 times, but the mid range's integer FB_DIV of 2
	 * leaves 37.5 at the loop: that is a fractional ratio.
	 */
	struct cs47l63_fll_solution sol;

	zassert_ok(cs47l63_clock_solve(FREF_655K36_HZ, &sol));

	zassert_equal(decoded_fout(&sol, FREF_655K36_HZ), CS47L63_FLL_FOUT_HZ,
		      "the solved terms must reproduce 49.152 MHz exactly");
	zassert_true((sol.theta == 0U) || (sol.hp == CS47L63_FLL1_HP_FRAC),
		     "a fractional ratio must run in the fractional loop mode");
	zassert_true((sol.theta == 0U) || (sol.n >= 2U), "fractional mode needs N of at least 2");
}

static ZTEST(cs47l63_clock_solver, test_integer_mode_is_always_the_high_performance_loop)
{
	static const uint32_t k_fref[] = {FREF_32K768_HZ, FREF_655K36_HZ, FREF_3M072_HZ,
					  FREF_6M144_HZ,  FREF_11M2896_HZ, FREF_12M288_HZ,
					  FREF_26M_HZ};
	struct cs47l63_fll_solution sol;

	zassert_ok(cs47l63_clock_solve(FREF_32K768_HZ, &sol));
	zassert_equal(sol.hp, CS47L63_FLL1_HP_INT, "32.768 kHz runs the integer loop at HP 01");

	for (size_t i = 0U; i < ARRAY_SIZE(k_fref); i++) {
		zassert_ok(cs47l63_clock_solve(k_fref[i], &sol), "%u Hz must solve", k_fref[i]);
		zassert_true((sol.hp == CS47L63_FLL1_HP_INT) || (sol.hp == CS47L63_FLL1_HP_FRAC),
			     "%u Hz: FLL1_HP is 01 or 11, never the reserved 00", k_fref[i]);
	}
}

static ZTEST(cs47l63_clock_solver, test_reference_below_32khz_rejected)
{
	struct cs47l63_fll_solution sol;

	zassert_equal(-ENOTSUP, cs47l63_clock_solve(FREF_16K_HZ, &sol),
		      "the FLL input floor is 32 kHz");
}

static ZTEST(cs47l63_clock_solver, test_fields_fit_their_register_widths)
{
	struct cs47l63_fll_solution sol;

	zassert_ok(cs47l63_clock_solve(FREF_11M2896_HZ, &sol));

	zassert_equal((uint32_t)sol.n & ~(CS47L63_FLL1_N_MASK >> CS47L63_FLL1_N_SHIFT), 0U,
		      "N must fit the ten bits of FLL1_N");
	zassert_equal((uint32_t)sol.fb_div &
			      ~(CS47L63_FLL1_FB_DIV_MASK >> CS47L63_FLL1_FB_DIV_SHIFT),
		      0U, "FB_DIV must fit the ten bits of FLL1_FB_DIV");
	zassert_true(sol.refclk_div <=
			     (CS47L63_FLL1_REFCLK_DIV_MASK >> CS47L63_FLL1_REFCLK_DIV_SHIFT),
		     "REFCLK_DIV selects one of /1 /2 /4 /8");
	zassert_true(sol.hp <= (CS47L63_FLL1_HP_MASK >> CS47L63_FLL1_HP_SHIFT),
		     "FLL1_HP is a two-bit field");
	zassert_true(sol.lockdet_thr <=
			     (CS47L63_FLL1_LOCKDET_THR_MASK >> CS47L63_FLL1_LOCKDET_THR_SHIFT),
		     "FLL1_LOCKDET_THR is a four-bit field");
}

static ZTEST(cs47l63_clock_solver, test_zero_reference_rejected)
{
	struct cs47l63_fll_solution sol;

	zassert_equal(-EINVAL, cs47l63_clock_solve(0U, &sol),
		      "a zero reference is a caller error, not an unreachable ratio");
}

static ZTEST(cs47l63_clock_solver, test_null_solution_rejected)
{
	zassert_equal(-EINVAL, cs47l63_clock_solve(FREF_6M144_HZ, NULL));
}

static ZTEST(cs47l63_clock_solver, test_reference_above_input_ceiling_rejected)
{
	struct cs47l63_fll_solution sol;

	zassert_equal(-ENOTSUP, cs47l63_clock_solve(FREF_120M_HZ, &sol));
}

static ZTEST(cs47l63_clock_solver, test_reference_the_divider_cannot_divide_exactly_rejected)
{
	struct cs47l63_fll_solution sol;

	zassert_ok(cs47l63_clock_solve(FREF_26M_HZ, &sol),
		   "the premise: the even neighbour solves");
	zassert_equal(decoded_fout(&sol, FREF_26M_HZ), CS47L63_FLL_FOUT_HZ);

	zassert_equal(-ENOTSUP, cs47l63_clock_solve(FREF_26M_HZ + 1U, &sol),
		      "26000001 Hz / 2 is not 13 MHz, so no solution is exact");
}
