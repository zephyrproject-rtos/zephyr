/*
 * Copyright (c) 2026 Powersoft S.p.A.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <math.h>

#include <zephyr/ztest.h>

#include "ptp_clock_nxp_enet_rate_math.h"

#define INC_CORR_MAX   127
#define COR_MAX        0x7FFFFFFFU
#define NSEC_PER_SEC_F 1000000000.0

struct clock_case {
	const char *name;
	uint32_t clock_hz;
	/* Worst error of the average tick over +-200 ppm, with margin */
	double max_residual_ppm;
};

/*
 * The error depends on the fractional part of the tick: a fraction close to a simple ratio, as
 * 2/3 for 24 MHz, leaves few periods to choose from. The limits are the measured worst cases
 * with some margin.
 */
static const struct clock_case clock_cases[] = {
	{"audio_pll_div2", 196608000, 5.0}, {"audio_pll_div4", 98304000, 5.0},
	{"osc_24m", 24000000, 40.0},        {"sys_pll1_div2_div5", 100000000, 0.1},
	{"whole_tick_25m", 25000000, 0.1},  {"arbitrary_61m", 61003000, 6.0},
};

static int inc_for_clock(uint32_t clock_hz)
{
	return (int)(NSEC_PER_SEC_F / (double)clock_hz);
}

/* Average tick of the timer, see ptp_clock_nxp_enet_find_correction() */
static double avg_ns_per_tick(int inc, int inc_corr, uint32_t cor)
{
	if (cor == 0U) {
		return (double)inc;
	}

	return (double)inc + (double)(inc_corr - inc) / (double)(cor + 1U);
}

static double residual_ppm(int inc, int inc_corr, uint32_t cor, double target_ns)
{
	return (avg_ns_per_tick(inc, inc_corr, cor) - target_ns) / target_ns * 1.0e6;
}

/*
 * The period is not linear in the correction, so the closer of the two integer periods around
 * the exact one is not always the rounded one. With inc = 126 only INC_CORR = 127 can be used
 * and the exact period is 2.45, but the periods 2 and 3 are equally close at 2.4, so 3 wins.
 * ATCOR is the period minus one, 2.
 */
ZTEST(ptp_clock_nxp_enet_rate_math, test_closer_neighbour_period)
{
	int inc = 126;
	int inc_corr;
	uint32_t cor;
	int ret = ptp_clock_nxp_enet_find_correction(inc, (double)inc + 1.0 / 2.45, INC_CORR_MAX,
						     COR_MAX, &inc_corr, &cor);

	zassert_equal(ret, 0, "failed with %d", ret);
	zassert_equal(cor, 2, "ATCOR %u, the period 3 is the closer one", cor);
}

/*
 * ATCOR is the number of ticks of the period minus one. At 98.304 MHz and a ratio of 1.0 the
 * best pair is a distance of 54 over 313 ticks, and at 196.608 MHz 27 over 313 ticks.
 */
ZTEST(ptp_clock_nxp_enet_rate_math, test_register_is_period_minus_one)
{
	static const struct {
		uint32_t clock_hz;
		int inc_corr;
	} expected[] = {
		{98304000, 64},
		{196608000, 32},
	};

	for (size_t i = 0; i < ARRAY_SIZE(expected); i++) {
		int inc = inc_for_clock(expected[i].clock_hz);
		int inc_corr;
		uint32_t cor;
		int ret = ptp_clock_nxp_enet_find_correction(
			inc, NSEC_PER_SEC_F / (double)expected[i].clock_hz, INC_CORR_MAX, COR_MAX,
			&inc_corr, &cor);

		zassert_equal(ret, 0, "failed with %d", ret);
		zassert_equal(inc_corr, expected[i].inc_corr, "INC_CORR %d at %u Hz", inc_corr,
			      expected[i].clock_hz);
		zassert_equal(cor, 312, "ATCOR %u at %u Hz, the period is 313 ticks", cor,
			      expected[i].clock_hz);
	}
}

/* ATCOR = 0 turns the correction off, so the shortest period is 2 and not 1 */
ZTEST(ptp_clock_nxp_enet_rate_math, test_shortest_period_is_two)
{
	int inc = 126;
	int inc_corr;
	uint32_t cor;
	int ret = ptp_clock_nxp_enet_find_correction(inc, (double)inc + 0.9, INC_CORR_MAX, COR_MAX,
						     &inc_corr, &cor);

	zassert_equal(ret, 0, "failed with %d", ret);
	zassert_equal(inc_corr, 127, "INC_CORR %d", inc_corr);
	zassert_equal(cor, 1, "ATCOR %u, 0 would drop the correction", cor);
}

/* A tick of a whole number of ns needs no correction */
ZTEST(ptp_clock_nxp_enet_rate_math, test_whole_tick_is_not_corrected)
{
	static const uint32_t clock_hz[] = {100000000, 25000000};

	for (size_t i = 0; i < ARRAY_SIZE(clock_hz); i++) {
		int inc = inc_for_clock(clock_hz[i]);
		int inc_corr;
		uint32_t cor;
		int ret = ptp_clock_nxp_enet_find_correction(
			inc, NSEC_PER_SEC_F / (double)clock_hz[i], INC_CORR_MAX, COR_MAX, &inc_corr,
			&cor);

		zassert_equal(ret, 0, "failed with %d", ret);
		zassert_equal(cor, 0, "ATCOR %u at %u Hz", cor, clock_hz[i]);
		zassert_equal(inc_corr, inc, "INC_CORR %d at %u Hz", inc_corr, clock_hz[i]);
	}
}

/* A fraction too small for any period in ATCOR leaves the correction off */
ZTEST(ptp_clock_nxp_enet_rate_math, test_tiny_fraction_is_not_corrected)
{
	int inc = 10;
	int inc_corr;
	uint32_t cor;
	int ret = ptp_clock_nxp_enet_find_correction(inc, (double)inc + 1.0e-12, INC_CORR_MAX,
						     COR_MAX, &inc_corr, &cor);

	zassert_equal(ret, 0, "failed with %d", ret);
	zassert_equal(cor, 0, "ATCOR %u", cor);
	zassert_equal(inc_corr, inc, "INC_CORR %d", inc_corr);
}

ZTEST(ptp_clock_nxp_enet_rate_math, test_sweep_within_limit)
{
	for (size_t i = 0; i < ARRAY_SIZE(clock_cases); i++) {
		int inc = inc_for_clock(clock_cases[i].clock_hz);
		double nominal_ns = NSEC_PER_SEC_F / (double)clock_cases[i].clock_hz;

		for (double ppm = -200.0; ppm <= 200.0; ppm += 0.5) {
			double target_ns = nominal_ns * (1.0 + ppm * 1.0e-6);
			int inc_corr;
			uint32_t cor;
			int ret = ptp_clock_nxp_enet_find_correction(inc, target_ns, INC_CORR_MAX,
								     COR_MAX, &inc_corr, &cor);
			double residual;

			zassert_equal(ret, 0, "%s at %.1f ppm: failed with %d", clock_cases[i].name,
				      ppm, ret);

			residual = fabs(residual_ppm(inc, inc_corr, cor, target_ns));
			zassert_true(residual <= clock_cases[i].max_residual_ppm,
				     "%s at %.1f ppm: error of %.4f ppm over the limit of %.1f ppm",
				     clock_cases[i].name, ppm, residual,
				     clock_cases[i].max_residual_ppm);
		}
	}
}

ZTEST(ptp_clock_nxp_enet_rate_math, test_fields_stay_in_range)
{
	static const double offsets_ppm[] = {-200, -100, 0, 100, 200, -5000, 5000, -50000, 50000};

	for (size_t i = 0; i < ARRAY_SIZE(clock_cases); i++) {
		int inc = inc_for_clock(clock_cases[i].clock_hz);
		double nominal_ns = NSEC_PER_SEC_F / (double)clock_cases[i].clock_hz;

		for (size_t j = 0; j < ARRAY_SIZE(offsets_ppm); j++) {
			double target_ns = nominal_ns * (1.0 + offsets_ppm[j] * 1.0e-6);
			int inc_corr;
			uint32_t cor;
			int ret = ptp_clock_nxp_enet_find_correction(inc, target_ns, INC_CORR_MAX,
								     COR_MAX, &inc_corr, &cor);

			if (ret != 0) {
				continue;
			}

			zassert_true(inc_corr >= 0 && inc_corr <= INC_CORR_MAX,
				     "%s at %.0f ppm: INC_CORR %d", clock_cases[i].name,
				     offsets_ppm[j], inc_corr);
			zassert_true(cor <= COR_MAX, "%s at %.0f ppm: ATCOR %u",
				     clock_cases[i].name, offsets_ppm[j], cor);
			zassert_equal(cor == 0U, inc_corr == inc,
				      "%s at %.0f ppm: ATCOR 0 only with INC_CORR = INC",
				      clock_cases[i].name, offsets_ppm[j]);
		}
	}
}

/* There is nothing to search when INC has no room next to the target */
ZTEST(ptp_clock_nxp_enet_rate_math, test_no_room_returns_einval)
{
	int inc = INC_CORR_MAX;
	int inc_corr;
	uint32_t cor;
	int ret = ptp_clock_nxp_enet_find_correction(inc, (double)(inc + 1), INC_CORR_MAX, COR_MAX,
						     &inc_corr, &cor);

	zassert_equal(ret, -EINVAL, "got %d", ret);

	ret = ptp_clock_nxp_enet_find_correction(inc, (double)(inc - 1), INC_CORR_MAX, COR_MAX,
						 &inc_corr, &cor);
	zassert_equal(ret, 0, "got %d, the way down is open", ret);
}

ZTEST(ptp_clock_nxp_enet_rate_math, test_average_tick_rises_with_ratio)
{
	static const double offsets_ppm[] = {-200, -100, 0, 100, 200};

	for (size_t i = 0; i < ARRAY_SIZE(clock_cases); i++) {
		int inc = inc_for_clock(clock_cases[i].clock_hz);
		double nominal_ns = NSEC_PER_SEC_F / (double)clock_cases[i].clock_hz;
		double previous = -HUGE_VAL;

		for (size_t j = 0; j < ARRAY_SIZE(offsets_ppm); j++) {
			double target_ns = nominal_ns * (1.0 + offsets_ppm[j] * 1.0e-6);
			int inc_corr;
			uint32_t cor;
			int ret = ptp_clock_nxp_enet_find_correction(inc, target_ns, INC_CORR_MAX,
								     COR_MAX, &inc_corr, &cor);
			double average;

			zassert_equal(ret, 0, "%s at %.0f ppm: failed with %d", clock_cases[i].name,
				      offsets_ppm[j], ret);

			average = avg_ns_per_tick(inc, inc_corr, cor);
			zassert_true(average >= previous,
				     "%s at %.0f ppm: tick %.9f fell from %.9f",
				     clock_cases[i].name, offsets_ppm[j], average, previous);
			previous = average;
		}
	}
}

ZTEST_SUITE(ptp_clock_nxp_enet_rate_math, NULL, NULL, NULL, NULL, NULL);
