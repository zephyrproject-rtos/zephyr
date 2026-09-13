/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file test_clock_solver.c
 * @brief cs47l63_clock_solve(), the pure half of the FLL1 path
 *
 * The expected values here are the datasheet's, not the solver's. DS1249F2
 * section 4.10.7.4 is the authority for all three constraints exercised below:
 * the FLL output must be in the 45 to 50 MHz range, it must be exactly
 * 49.152 MHz when the loop is the SYSCLK source for the 48 kHz sample-rate
 * family, and the reference divider must bring the reference to 13 MHz or
 * below (section 4.10.7.3).
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/ztest.h>

#include "cs47l63_clock.h"
#include "cs47l63_regs.h"

/** The 44.1 kHz family's SYSCLK, legal on the part but not in this driver. */
#define FOUT_45M1584 45158400U

/** The FLL output range, DS1249F2 section 4.10.7.4. */
#define FOUT_BELOW_RANGE 44000000U
#define FOUT_ABOVE_RANGE 51000000U

/** The reference-divider ceiling, DS1249F2 section 4.10.7.3. */
#define FREF_MAX_HZ 13000000U

/**
 * @brief Reconstruct the output frequency from the solved fields alone.
 *
 * FFLL = (FREF >> FLL1_REFCLK_DIV) * FLL1_FB_DIV * (N + THETA / LAMBDA), so a
 * solution that decodes back to the requested frequency has named the ratio
 * the caller asked for rather than one near it.
 */
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

	zassert_ok(cs47l63_clock_solve(6144000, CS47L63_FLL_FOUT_HZ, CS47L63_FLL_SRC_MCLK1, &sol));

	zassert_equal(sol.refclk_src, CS47L63_FLL_SRC_MCLK1,
		      "the reference source is carried through unchanged");
	zassert_equal(sol.refclk_div, 0, "6.144 MHz is already below the 13 MHz input ceiling");
	zassert_equal(decoded_fout(&sol, 6144000), CS47L63_FLL_FOUT_HZ,
		      "the solved terms must reproduce 49.152 MHz exactly");
	zassert_equal(sol.sysclk_freq, CS47L63_SYSCLK_FREQ_49M152,
		      "SYSCLK_FREQ must name 49.152 MHz, DS1249F2 table 4-48");
}

ZTEST(cs47l63_clock_solver, test_integer_references_decode_back_exactly)
{
	/* Every reference here divides 49.152 MHz exactly, so each must solve
	 * with a zero fractional term and decode straight back.
	 */
	static const uint32_t k_fref[] = {32768, 3072000, 6144000, 12288000};

	for (size_t i = 0; i < ARRAY_SIZE(k_fref); i++) {
		struct cs47l63_fll_solution sol;

		zassert_ok(cs47l63_clock_solve(k_fref[i], CS47L63_FLL_FOUT_HZ,
					       CS47L63_FLL_SRC_MCLK1, &sol),
			   "%u Hz divides 49.152 MHz exactly and must solve", k_fref[i]);
		zassert_equal(sol.theta, 0, "%u Hz needs no fractional term", k_fref[i]);
		zassert_equal(decoded_fout(&sol, k_fref[i]), CS47L63_FLL_FOUT_HZ,
			      "%u Hz decoded to the wrong output", k_fref[i]);
	}
}

ZTEST(cs47l63_clock_solver, test_fractional_reference_decodes_back_exactly)
{
	/* 11.2896 MHz is the 44.1 kHz family's master clock: it does not divide
	 * 49.152 MHz, so the ratio has to be carried in THETA / LAMBDA. The
	 * exact ratio is 640 / 147, that is N = 4 with 52 / 147 left over.
	 */
	struct cs47l63_fll_solution sol;

	zassert_ok(cs47l63_clock_solve(11289600, CS47L63_FLL_FOUT_HZ, CS47L63_FLL_SRC_MCLK1, &sol));

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

	zassert_ok(cs47l63_clock_solve(11289600, CS47L63_FLL_FOUT_HZ, CS47L63_FLL_SRC_MCLK1, &sol));

	zassert_equal((uint32_t)sol.n & ~(CS47L63_FLL1_N_MASK >> CS47L63_FLL1_N_SHIFT), 0,
		      "N must fit the ten bits of FLL1_N");
	zassert_equal((uint32_t)sol.fb_div &
			      ~(CS47L63_FLL1_FB_DIV_MASK >> CS47L63_FLL1_FB_DIV_SHIFT),
		      0, "FB_DIV must fit the ten bits of FLL1_FB_DIV");
	zassert_true(sol.refclk_div <= 3, "REFCLK_DIV selects one of /1 /2 /4 /8");
	zassert_true(sol.hp <= 3, "FLL1_HP is a two-bit field");
	zassert_true(sol.lockdet_thr <= 0xF, "FLL1_LOCKDET_THR is a four-bit field");
}

ZTEST(cs47l63_clock_solver, test_output_the_part_forbids_as_sysclk_source_rejected)
{
	struct cs47l63_fll_solution sol;

	/* DS1249F2 section 4.10.7.4: an FLL feeding SYSCLK must be at exactly
	 * 49.152 MHz for the 48 kHz sample-rate family. 45.1584 MHz is the
	 * 44.1 kHz family's value and needs SYSCLK_FRAC set, which this driver
	 * never does, so it has to be refused rather than programmed.
	 */
	zassert_equal(-ENOTSUP,
		      cs47l63_clock_solve(6144000, FOUT_45M1584, CS47L63_FLL_SRC_MCLK1, &sol),
		      "45.1584 MHz belongs to the 44.1 kHz family this driver does not select");

	zassert_equal(-ENOTSUP,
		      cs47l63_clock_solve(6144000, FOUT_BELOW_RANGE, CS47L63_FLL_SRC_MCLK1, &sol),
		      "below the 45 MHz floor of the FLL output range");

	zassert_equal(-ENOTSUP,
		      cs47l63_clock_solve(6144000, FOUT_ABOVE_RANGE, CS47L63_FLL_SRC_MCLK1, &sol),
		      "above the 50 MHz ceiling of the FLL output range");

	zassert_equal(-ENOTSUP, cs47l63_clock_solve(6144000, 24576000, CS47L63_FLL_SRC_MCLK1, &sol),
		      "24.576 MHz is a SYSCLK_FREQ code but not a legal FLL output");
}

ZTEST(cs47l63_clock_solver, test_zero_reference_rejected)
{
	struct cs47l63_fll_solution sol;

	zassert_equal(-EINVAL,
		      cs47l63_clock_solve(0, CS47L63_FLL_FOUT_HZ, CS47L63_FLL_SRC_MCLK1, &sol),
		      "a zero reference is a caller error, not an unreachable ratio");
}

ZTEST(cs47l63_clock_solver, test_zero_output_rejected)
{
	struct cs47l63_fll_solution sol;

	zassert_equal(-EINVAL, cs47l63_clock_solve(6144000, 0, CS47L63_FLL_SRC_MCLK1, &sol));
}

ZTEST(cs47l63_clock_solver, test_null_solution_rejected)
{
	zassert_equal(-EINVAL, cs47l63_clock_solve(6144000, CS47L63_FLL_FOUT_HZ,
						   CS47L63_FLL_SRC_MCLK1, NULL));
}

ZTEST(cs47l63_clock_solver, test_reference_above_input_ceiling_rejected)
{
	struct cs47l63_fll_solution sol;

	/* 120 MHz is still above the 13 MHz input ceiling after the deepest
	 * divide the reference predivider offers, /8.
	 */
	zassert_true((120000000U >> 3) > FREF_MAX_HZ, "the premise: /8 does not bring it in range");
	zassert_equal(-ENOTSUP, cs47l63_clock_solve(120000000, CS47L63_FLL_FOUT_HZ,
						    CS47L63_FLL_SRC_MCLK1, &sol));
}
