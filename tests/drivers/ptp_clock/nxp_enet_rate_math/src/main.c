/*
 * Copyright (c) 2026 Powersoft S.p.A.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <math.h>
#include <stdlib.h>

#include <zephyr/ztest.h>

#include "ptp_clock_nxp_enet_rate_math.h"

#define INC_CORR_MAX   127
#define COR_MAX        0x7FFFFFFFU
#define NSEC_PER_SEC_U 1000000000U
/* Same relative tolerance as the driver */
#define RATE_TOLERANCE 1.0e-7

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
	{"osc_24m", 24000000, 40.0},        {"sys_pll1_div2_div5", 100000000, 0.15},
	{"whole_tick_25m", 25000000, 0.15}, {"arbitrary_61m", 61003000, 6.0},
};

/* Average tick of the timer, see ptp_clock_nxp_enet_find_correction() */
static double avg_ns_per_tick(int inc, int inc_corr, uint32_t cor)
{
	if (cor == 0U) {
		return (double)inc;
	}

	return (double)inc + (double)(inc_corr - inc) / (double)(cor + 1U);
}

/* The search for a clock and a rate offset, with the inputs the driver computes */
static int find_for_clock(uint32_t clock_hz, double offset_ppm, int *inc, int *inc_corr,
			  uint32_t *cor)
{
	double tick_ns = (double)NSEC_PER_SEC_U / (double)clock_hz;
	double frac_ns = (double)(NSEC_PER_SEC_U % clock_hz) / (double)clock_hz +
			 tick_ns * offset_ppm * 1.0e-6;

	*inc = (int)(NSEC_PER_SEC_U / clock_hz);

	return ptp_clock_nxp_enet_find_correction(*inc, frac_ns, tick_ns * RATE_TOLERANCE,
						  INC_CORR_MAX, COR_MAX, inc_corr, cor);
}

static double residual_ppm(uint32_t clock_hz, double offset_ppm, int inc, int inc_corr,
			   uint32_t cor)
{
	double target_ns = (double)NSEC_PER_SEC_U / (double)clock_hz * (1.0 + offset_ppm * 1.0e-6);

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
	int inc_corr;
	uint32_t cor;
	int ret = ptp_clock_nxp_enet_find_correction(126, 1.0 / 2.45, 0.0, INC_CORR_MAX, COR_MAX,
						     &inc_corr, &cor);

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
		int inc;
		int inc_corr;
		uint32_t cor;
		int ret = find_for_clock(expected[i].clock_hz, 0.0, &inc, &inc_corr, &cor);

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
	int inc_corr;
	uint32_t cor;
	int ret = ptp_clock_nxp_enet_find_correction(126, 0.5, 0.0, INC_CORR_MAX, COR_MAX,
						     &inc_corr, &cor);

	zassert_equal(ret, 0, "failed with %d", ret);
	zassert_equal(inc_corr, 127, "INC_CORR %d", inc_corr);
	zassert_equal(cor, 1, "ATCOR %u, 0 would drop the correction", cor);
}

/*
 * A shift beyond half of the room next to INC needs a period shorter than 2 ticks. It is
 * rejected rather than left uncorrected, and so is a value that is not a number. The outputs
 * are left alone.
 */
ZTEST(ptp_clock_nxp_enet_rate_math, test_unreachable_shift_returns_einval)
{
	int inc;
	int inc_corr = -1;
	uint32_t cor = 12345U;
	int ret;

	ret = ptp_clock_nxp_enet_find_correction(126, 0.9, 0.0, INC_CORR_MAX, COR_MAX, &inc_corr,
						 &cor);
	zassert_equal(ret, -EINVAL, "inc 126, shift 0.9: got %d", ret);

	/* 8 MHz is a 125 ns tick, 4% more is 130 ns: INC_CORR would have to be 135 */
	ret = find_for_clock(8000000, 40000.0, &inc, &inc_corr, &cor);
	zassert_equal(ret, -EINVAL, "8 MHz at +4%%: got %d", ret);

	ret = ptp_clock_nxp_enet_find_correction(10, (double)NAN, 0.0, INC_CORR_MAX, COR_MAX,
						 &inc_corr, &cor);
	zassert_equal(ret, -EINVAL, "NaN: got %d", ret);

	zassert_equal(inc_corr, -1, "INC_CORR was written");
	zassert_equal(cor, 12345U, "ATCOR was written");
}

/* A tick of a whole number of ns needs no correction */
ZTEST(ptp_clock_nxp_enet_rate_math, test_whole_tick_is_not_corrected)
{
	static const uint32_t clock_hz[] = {100000000, 25000000};

	for (size_t i = 0; i < ARRAY_SIZE(clock_hz); i++) {
		int inc;
		int inc_corr;
		uint32_t cor;
		int ret = find_for_clock(clock_hz[i], 0.0, &inc, &inc_corr, &cor);

		zassert_equal(ret, 0, "failed with %d", ret);
		zassert_equal(cor, 0, "ATCOR %u at %u Hz", cor, clock_hz[i]);
		zassert_equal(inc_corr, inc, "INC_CORR %d at %u Hz", inc_corr, clock_hz[i]);
	}
}

/* A fraction too small for any period in ATCOR leaves the correction off */
ZTEST(ptp_clock_nxp_enet_rate_math, test_tiny_fraction_is_not_corrected)
{
	int inc_corr;
	uint32_t cor;
	int ret = ptp_clock_nxp_enet_find_correction(10, 1.0e-12, 0.0, INC_CORR_MAX, COR_MAX,
						     &inc_corr, &cor);

	zassert_equal(ret, 0, "failed with %d", ret);
	zassert_equal(cor, 0, "ATCOR %u", cor);
	zassert_equal(inc_corr, 10, "INC_CORR %d", inc_corr);
}

/*
 * A locked servo asks for changes of a few ppb. On a whole tick they are reached exactly with
 * a step of 1 ns and a long period, which keeps the time smooth.
 */
ZTEST(ptp_clock_nxp_enet_rate_math, test_small_change_uses_one_ns_step)
{
	static const double offsets_ppm[] = {0.001, 0.005, 0.01, 0.02, -0.01, -0.02};

	for (size_t i = 0; i < ARRAY_SIZE(offsets_ppm); i++) {
		int inc;
		int inc_corr;
		uint32_t cor;
		int ret = find_for_clock(100000000, offsets_ppm[i], &inc, &inc_corr, &cor);

		zassert_equal(ret, 0, "%.3f ppm: failed with %d", offsets_ppm[i], ret);
		zassert_equal(abs(inc_corr - inc), 1, "%.3f ppm: INC_CORR %d, ATCOR %u",
			      offsets_ppm[i], inc_corr, cor);
	}
}

/*
 * Each corrected tick moves the time by INC_CORR - INC at once. On a whole tick the search
 * stops at the first pair within the tolerance, which keeps that step small. Without the
 * tolerance it goes above 100 ns at 100 MHz.
 */
ZTEST(ptp_clock_nxp_enet_rate_math, test_step_stays_small_on_whole_tick)
{
	static const struct {
		uint32_t clock_hz;
		int max_step;
	} cases[] = {
		{100000000, 2},
		{25000000, 8},
	};

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		for (double ppm = -200.0; ppm <= 200.0; ppm += 0.5) {
			int inc;
			int inc_corr;
			uint32_t cor;
			int ret = find_for_clock(cases[i].clock_hz, ppm, &inc, &inc_corr, &cor);

			zassert_equal(ret, 0, "%u Hz at %.1f ppm: failed with %d",
				      cases[i].clock_hz, ppm, ret);
			zassert_true(abs(inc_corr - inc) <= cases[i].max_step,
				     "%u Hz at %.1f ppm: step of %d ns", cases[i].clock_hz, ppm,
				     inc_corr - inc);
		}
	}
}

ZTEST(ptp_clock_nxp_enet_rate_math, test_sweep_within_limit)
{
	for (size_t i = 0; i < ARRAY_SIZE(clock_cases); i++) {
		for (double ppm = -200.0; ppm <= 200.0; ppm += 0.5) {
			int inc;
			int inc_corr;
			uint32_t cor;
			int ret =
				find_for_clock(clock_cases[i].clock_hz, ppm, &inc, &inc_corr, &cor);
			double residual;

			zassert_equal(ret, 0, "%s at %.1f ppm: failed with %d", clock_cases[i].name,
				      ppm, ret);

			residual = fabs(
				residual_ppm(clock_cases[i].clock_hz, ppm, inc, inc_corr, cor));
			zassert_true(residual <= clock_cases[i].max_residual_ppm,
				     "%s at %.1f ppm: error of %.4f ppm over the limit of %.1f ppm",
				     clock_cases[i].name, ppm, residual,
				     clock_cases[i].max_residual_ppm);
		}
	}
}

/*
 * Large offsets are reached coarsely, but in the right direction: the error stays well below
 * the offset itself.
 */
ZTEST(ptp_clock_nxp_enet_rate_math, test_large_offsets)
{
	static const double offsets_ppm[] = {-50000, -5000, 5000, 50000};

	for (size_t i = 0; i < ARRAY_SIZE(clock_cases); i++) {
		for (size_t j = 0; j < ARRAY_SIZE(offsets_ppm); j++) {
			int inc;
			int inc_corr;
			uint32_t cor;
			int ret = find_for_clock(clock_cases[i].clock_hz, offsets_ppm[j], &inc,
						 &inc_corr, &cor);
			double residual;

			zassert_equal(ret, 0, "%s at %.0f ppm: failed with %d", clock_cases[i].name,
				      offsets_ppm[j], ret);
			zassert_true(inc_corr >= 0 && inc_corr <= INC_CORR_MAX,
				     "%s at %.0f ppm: INC_CORR %d", clock_cases[i].name,
				     offsets_ppm[j], inc_corr);
			zassert_true(cor <= COR_MAX, "%s at %.0f ppm: ATCOR %u",
				     clock_cases[i].name, offsets_ppm[j], cor);

			residual = fabs(residual_ppm(clock_cases[i].clock_hz, offsets_ppm[j], inc,
						     inc_corr, cor));
			zassert_true(residual <= 500.0, "%s at %.0f ppm: error of %.1f ppm",
				     clock_cases[i].name, offsets_ppm[j], residual);
		}
	}
}

/* There is nothing to search when INC has no room next to the target */
ZTEST(ptp_clock_nxp_enet_rate_math, test_no_room_returns_einval)
{
	int inc_corr;
	uint32_t cor;
	int ret = ptp_clock_nxp_enet_find_correction(INC_CORR_MAX, 0.3, 0.0, INC_CORR_MAX, COR_MAX,
						     &inc_corr, &cor);

	zassert_equal(ret, -EINVAL, "got %d", ret);

	ret = ptp_clock_nxp_enet_find_correction(INC_CORR_MAX, -0.3, 0.0, INC_CORR_MAX, COR_MAX,
						 &inc_corr, &cor);
	zassert_equal(ret, 0, "got %d, the way down is open", ret);
}

ZTEST(ptp_clock_nxp_enet_rate_math, test_average_tick_rises_with_ratio)
{
	static const double offsets_ppm[] = {-200, -100, 0, 100, 200};

	for (size_t i = 0; i < ARRAY_SIZE(clock_cases); i++) {
		double previous = 0.0;

		for (size_t j = 0; j < ARRAY_SIZE(offsets_ppm); j++) {
			int inc;
			int inc_corr;
			uint32_t cor;
			int ret = find_for_clock(clock_cases[i].clock_hz, offsets_ppm[j], &inc,
						 &inc_corr, &cor);
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
