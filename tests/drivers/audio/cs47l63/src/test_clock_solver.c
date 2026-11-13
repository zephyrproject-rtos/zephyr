/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file test_clock_solver.c
 * @brief The FLL1 terms configure() solves, read back from the registers across references
 *
 * Expected values are DS1249F2's: FLL output 45 to 50 MHz, exactly 49.152 MHz for SYSCLK in
 * the 48 kHz family (section 4.10.7.4), reference at most 13 MHz (section 4.10.7.3).
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/ztest.h>

#include "cs47l63.h"

#include "cs47l63_emul.h"

#define CODEC DEVICE_DT_GET(DT_NODELABEL(cs47l63_clock))
#define EMUL  EMUL_DT_GET(DT_NODELABEL(cs47l63_clock))

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

#define FLL1_HALF_MASK 0xFFFFU

BUILD_ASSERT((FREF_120M_HZ / REFDIV_MAX) > FREF_MAX_HZ,
	     "the premise: /8 does not bring it in range");

struct fll_terms {
	uint32_t refclk_div;
	uint32_t hp;
	uint32_t n;
	uint32_t fb_div;
	uint32_t lambda;
	uint32_t theta;
};

static int configure_at(uint32_t mclk_hz)
{
	struct audio_codec_cfg cfg = cs47l63_test_cfg(AUDIO_ROUTE_PLAYBACK_CAPTURE);

	cs47l63_emul_reset(EMUL);
	cfg.mclk_freq = mclk_hz;

	return audio_codec_configure(CODEC, &cfg);
}

static struct fll_terms read_terms(void)
{
	const uint32_t ctrl2 = cs47l63_emul_get_reg(EMUL, CS47L63_FLL1_CONTROL2);
	const uint32_t ctrl3 = cs47l63_emul_get_reg(EMUL, CS47L63_FLL1_CONTROL3);
	const uint32_t ctrl4 = cs47l63_emul_get_reg(EMUL, CS47L63_FLL1_CONTROL4);

	return (struct fll_terms){
		.refclk_div =
			(ctrl2 & CS47L63_FLL1_REFCLK_DIV_MASK) >> CS47L63_FLL1_REFCLK_DIV_SHIFT,
		.n = (ctrl2 & CS47L63_FLL1_N_MASK) >> CS47L63_FLL1_N_SHIFT,
		.lambda = (ctrl3 >> CS47L63_FLL1_LAMBDA_SHIFT) & FLL1_HALF_MASK,
		.theta = (ctrl3 >> CS47L63_FLL1_THETA_SHIFT) & FLL1_HALF_MASK,
		.hp = (ctrl4 & CS47L63_FLL1_HP_MASK) >> CS47L63_FLL1_HP_SHIFT,
		.fb_div = (ctrl4 & CS47L63_FLL1_FB_DIV_MASK) >> CS47L63_FLL1_FB_DIV_SHIFT,
	};
}

/** FFLL = (FREF >> FLL1_REFCLK_DIV) * FLL1_FB_DIV * (N + THETA / LAMBDA). */
static uint64_t decoded_fout(const struct fll_terms *t, uint32_t fref_hz)
{
	uint64_t fref = (uint64_t)fref_hz >> t->refclk_div;
	uint64_t num = ((uint64_t)t->n * t->lambda) + t->theta;

	zassert_not_equal(t->lambda, 0U, "LAMBDA is the divisor of the fractional term");
	zassert_true(fref <= FREF_MAX_HZ, "REFCLK_DIV must bring the reference to 13 MHz or below");

	return fref * t->fb_div * num / t->lambda;
}

static void clock_solver_before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	cs47l63_emul_reset(EMUL);
}

ZTEST_SUITE(cs47l63_clock_solver, NULL, NULL, clock_solver_before_each, NULL, NULL);

static ZTEST(cs47l63_clock_solver, test_nominal_6m144_to_49m152)
{
	struct fll_terms t;

	zassert_ok(configure_at(FREF_6M144_HZ));
	t = read_terms();

	zassert_equal(t.refclk_div, 0U, "6.144 MHz is already below the 13 MHz input ceiling");
	zassert_equal(decoded_fout(&t, FREF_6M144_HZ), CS47L63_FLL_FOUT_HZ,
		      "the solved terms must reproduce 49.152 MHz exactly");
}

static ZTEST(cs47l63_clock_solver, test_integer_references_decode_back_exactly)
{
	/* Each reference divides 49.152 MHz exactly: no fractional term. */
	static const uint32_t k_fref[] = {FREF_32K768_HZ, FREF_3M072_HZ, FREF_6M144_HZ,
					  FREF_12M288_HZ};

	for (size_t i = 0U; i < ARRAY_SIZE(k_fref); i++) {
		struct fll_terms t;

		zassert_ok(configure_at(k_fref[i]),
			   "%u Hz divides 49.152 MHz exactly and must solve", k_fref[i]);
		t = read_terms();
		zassert_equal(t.theta, 0U, "%u Hz needs no fractional term", k_fref[i]);
		zassert_equal(decoded_fout(&t, k_fref[i]), CS47L63_FLL_FOUT_HZ,
			      "%u Hz decoded to the wrong output", k_fref[i]);
	}
}

static ZTEST(cs47l63_clock_solver, test_fractional_reference_decodes_back_exactly)
{
	/* 11.2896 MHz does not divide 49.152 MHz: the ratio is 640 / 147, N = 4 plus 52 / 147. */
	struct fll_terms t;

	zassert_ok(configure_at(FREF_11M2896_HZ));
	t = read_terms();

	zassert_not_equal(t.theta, 0U, "a non-dividing reference needs a fractional term");
	zassert_equal(t.n, EXPECT_FRAC_N, "the integer part of 640 / 147 is 4");
	zassert_equal(t.lambda, EXPECT_FRAC_LAMBDA,
		      "the ratio in lowest terms has 147 as its denominator");
	zassert_equal(t.theta, EXPECT_FRAC_THETA, "the remainder of 640 / 147 is 52");
	zassert_equal(decoded_fout(&t, FREF_11M2896_HZ), CS47L63_FLL_FOUT_HZ,
		      "N + THETA / LAMBDA must decode back to 49.152 MHz exactly");
}

static ZTEST(cs47l63_clock_solver, test_integer_ratio_split_by_fb_div_is_not_programmed_as_integer)
{
	/* 655.36 kHz divides 49.152 MHz 75 times, but the mid range's integer FB_DIV of 2
	 * leaves 37.5 at the loop: that is a fractional ratio.
	 */
	struct fll_terms t;

	zassert_ok(configure_at(FREF_655K36_HZ));
	t = read_terms();

	zassert_equal(decoded_fout(&t, FREF_655K36_HZ), CS47L63_FLL_FOUT_HZ,
		      "the solved terms must reproduce 49.152 MHz exactly");
	zassert_true((t.theta == 0U) || (t.hp == CS47L63_FLL1_HP_FRAC),
		     "a fractional ratio must run in the fractional loop mode");
	zassert_true((t.theta == 0U) || (t.n >= 2U), "fractional mode needs N of at least 2");
}

static ZTEST(cs47l63_clock_solver, test_integer_mode_is_always_the_high_performance_loop)
{
	static const uint32_t k_fref[] = {FREF_32K768_HZ, FREF_655K36_HZ, FREF_3M072_HZ,
					  FREF_6M144_HZ,  FREF_11M2896_HZ, FREF_12M288_HZ,
					  FREF_26M_HZ};
	struct fll_terms t;

	zassert_ok(configure_at(FREF_32K768_HZ));
	t = read_terms();
	zassert_equal(t.hp, CS47L63_FLL1_HP_INT, "32.768 kHz runs the integer loop at HP 01");

	for (size_t i = 0U; i < ARRAY_SIZE(k_fref); i++) {
		zassert_ok(configure_at(k_fref[i]), "%u Hz must solve", k_fref[i]);
		t = read_terms();
		zassert_true((t.hp == CS47L63_FLL1_HP_INT) || (t.hp == CS47L63_FLL1_HP_FRAC),
			     "%u Hz: FLL1_HP is 01 or 11, never the reserved 00", k_fref[i]);
	}
}

static ZTEST(cs47l63_clock_solver, test_reference_below_32khz_rejected)
{
	zassert_equal(-ENOTSUP, configure_at(FREF_16K_HZ), "the FLL input floor is 32 kHz");
	zassert_equal(cs47l63_emul_xfer_count(EMUL), 0U, "refused before the part is touched");
}

static ZTEST(cs47l63_clock_solver, test_zero_reference_rejected)
{
	zassert_equal(-EINVAL, configure_at(0U),
		      "a zero reference is a caller error, not an unreachable ratio");
	zassert_equal(cs47l63_emul_xfer_count(EMUL), 0U, "refused before the part is touched");
}

static ZTEST(cs47l63_clock_solver, test_reference_above_input_ceiling_rejected)
{
	zassert_equal(-ENOTSUP, configure_at(FREF_120M_HZ));
	zassert_equal(cs47l63_emul_xfer_count(EMUL), 0U, "refused before the part is touched");
}

static ZTEST(cs47l63_clock_solver, test_reference_the_divider_cannot_divide_exactly_rejected)
{
	struct fll_terms t;

	zassert_ok(configure_at(FREF_26M_HZ), "the premise: the even neighbour solves");
	t = read_terms();
	zassert_equal(decoded_fout(&t, FREF_26M_HZ), CS47L63_FLL_FOUT_HZ);

	zassert_equal(-ENOTSUP, configure_at(FREF_26M_HZ + 1U),
		      "26000001 Hz / 2 is not 13 MHz, so no solution is exact");
	zassert_equal(cs47l63_emul_xfer_count(EMUL), 0U, "refused before the part is touched");
}
